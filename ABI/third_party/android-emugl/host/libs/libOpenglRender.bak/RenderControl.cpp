/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "RenderControl.h"

#include "DispatchTables.h"
#include "FbConfig.h"
#include "FenceSync.h"
#include "FrameBuffer.h"
#include "GLESVersionDetector.h"
#include "RenderContext.h"
#include "RenderThreadInfo.h"
#include "SyncThread.h"
#include "ChecksumCalculatorThreadInfo.h"
#include "OpenGLESDispatch/EGLDispatch.h"

#include "android/utils/debug.h"
#include "android/base/StringView.h"
#include "emugl/common/feature_control.h"
#include "emugl/common/lazy_instance.h"
#include "emugl/common/sync_device.h"
#include "emugl/common/dma_device.h"
#include "emugl/common/misc.h"
#include "emugl/common/thread.h"
#include "math.h"

#include <atomic>
#include <inttypes.h>
#include <string.h>

using android::base::AutoLock;
using android::base::Lock;

#define DEBUG_GRALLOC_SYNC 0
#define DEBUG_EGL_SYNC 0

#define RENDERCONTROL_DPRINT(...) do { \
    if (!VERBOSE_CHECK(gles)) { VERBOSE_ENABLE(gles); } \
    VERBOSE_TID_FUNCTION_DPRINT(gles, __VA_ARGS__); \
} while(0)

#if DEBUG_GRALLOC_SYNC
#define GRSYNC_DPRINT RENDERCONTROL_DPRINT
#else
#define GRSYNC_DPRINT(...)
#endif

#if DEBUG_EGL_SYNC
#define EGLSYNC_DPRINT RENDERCONTROL_DPRINT
#else
#define EGLSYNC_DPRINT(...)
#endif

// GrallocSync is a class that helps to reflect the behavior of
// grallock_lock/gralloc_unlock on the guest.
// If we don't use this, apps that use gralloc buffers (such as webcam)
// will have out of order frames,
// as GL calls from different threads in the guest
// are allowed to arrive at the host in any ordering.
class GrallocSync {
public:
    GrallocSync() {
        // Having in-order webcam frames is nice, but not at the cost
        // of potential deadlocks;
        // we need to be careful of what situations in which
        // we actually lock/unlock the gralloc color buffer.
        //
        // To avoid deadlock:
        // we require rcColorBufferCacheFlush to be called
        // whenever gralloc_lock is called on the guest,
        // and we require rcUpdateWindowColorBuffer to be called
        // whenever gralloc_unlock is called on the guest.
        //
        // Some versions of the system image optimize out
        // the call to rcUpdateWindowColorBuffer in the case of zero
        // width/height, but since we're using that as synchronization,
        // that lack of calling can lead to a deadlock on the host
        // in many situations
        // (switching camera sides, exiting benchmark apps, etc)
        // So, we put GrallocSync under the feature control.
        mEnabled = emugl_feature_is_enabled(android::featurecontrol::GrallocSync);

        // There are two potential tricky situations to handle:
        // a. Multiple users of gralloc buffers that all want to
        // call gralloc_lock. This is obeserved to happen on older APIs
        // (<= 19).
        // b. The pipe doesn't have to preserve ordering of the
        // gralloc_lock and gralloc_unlock commands themselves.
        //
        // To handle a), notice the situation is one of one type of uses
        // needing multiple locks that needs to exclude concurrent use
        // by another type of user. This maps well to a read/write lock,
        // where gralloc_lock and gralloc_unlock users are readers
        // and rcFlushWindowColorBuffer is the writer.
        // From the perspective of the host preparing and posting
        // buffers, these are indeed read/write operations.
        //
        // To handle b), we give up on locking when the state is observed
        // to be bad. lockState tracks how many color buffer locks there are.
        // If lockState < 0, it means we definitely have an unlock before lock
        // sort of situation, and should give up.
        lockState = 0;
    }

    // lockColorBufferPrepare is designed to handle
    // gralloc_lock/unlock requests, and uses the read lock.
    // When rcFlushWindowColorBuffer is called (when frames are posted),
    // we use the write lock (see GrallocSyncPostLock).
    void lockColorBufferPrepare() {
        int newLockState = ++lockState;
        if (mEnabled && newLockState == 1) {
            mGrallocColorBufferLock.lockRead();
        } else if (mEnabled) {
            GRSYNC_DPRINT("warning: recursive/multiple locks from guest!");
        }
    }
    void unlockColorBufferPrepare() {
        int newLockState = --lockState;
        if (mEnabled && newLockState == 0) mGrallocColorBufferLock.unlockRead();
    }
    android::base::ReadWriteLock mGrallocColorBufferLock;
private:
    bool mEnabled;
    std::atomic<int> lockState;
    DISALLOW_COPY_ASSIGN_AND_MOVE(GrallocSync);
};

class GrallocSyncPostLock : public android::base::AutoWriteLock {
public:
    GrallocSyncPostLock(GrallocSync& grallocsync) :
        android::base::AutoWriteLock(grallocsync.mGrallocColorBufferLock) { }
};

static ::emugl::LazyInstance<GrallocSync> sGrallocSync = LAZY_INSTANCE_INIT;

static const GLint rendererVersion = 1;

// GLAsyncSwap version history:
// "ANDROID_EMU_NATIVE_SYNC": original version
// "ANDROIDEMU_native_sync_v2": +cleanup of sync objects
// (We need all the different strings to not be prefixes of any other
// due to how they are checked for in the GL extensions on the guest)
static constexpr android::base::StringView kAsyncSwapStr = "ANDROID_EMU_native_sync_v2";

// DMA version history:
// "ANDROID_EMU_dma_v1": add dma device and rcUpdateColorBufferDMA and do
// yv12 conversion on the GPU
static constexpr android::base::StringView kDmaStr = "ANDROID_EMU_dma_v1";

// GLESDynamicVersion: up to 3.1 so far
static constexpr android::base::StringView kGLESDynamicVersion_2 = "ANDROID_EMU_gles_max_version_2";
static constexpr android::base::StringView kGLESDynamicVersion_3_0 = "ANDROID_EMU_gles_max_version_3_0";
static constexpr android::base::StringView kGLESDynamicVersion_3_1 = "ANDROID_EMU_gles_max_version_3_1";

static void rcTriggerWait(uint64_t glsync_ptr,
                          uint64_t thread_ptr,
                          uint64_t timeline);

static GLint rcGetRendererVersion()
{
    emugl_sync_register_trigger_wait(rcTriggerWait);

    sGrallocSync.ptr();
    return rendererVersion;
}

static EGLint rcGetEGLVersion(EGLint* major, EGLint* minor)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return EGL_FALSE;
    }
    *major = (EGLint)fb->getCaps().eglMajor;
    *minor = (EGLint)fb->getCaps().eglMinor;

    return EGL_TRUE;
}

static EGLint rcQueryEGLString(EGLenum name, void* buffer, EGLint bufferSize)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    const char *str = s_egl.eglQueryString(fb->getDisplay(), name);
    if (!str) {
        return 0;
    }

    std::string eglStr(str);
    if ((calcMaxVersionFromDispatch() >= GLES_DISPATCH_MAX_VERSION_3_0) &&
        eglStr.find("EGL_KHR_create_context") == std::string::npos) {
        eglStr += "EGL_KHR_create_context ";
    }

    int len = eglStr.size() + 1;
    if (!buffer || len > bufferSize) {
        return -len;
    }

    strcpy((char *)buffer, eglStr.c_str());
    return len;
}

static bool shouldEnableAsyncSwap() {
    bool isPhone;
    emugl::getAvdInfo(&isPhone, NULL);
    return emugl_feature_is_enabled(android::featurecontrol::GLAsyncSwap) &&
           emugl_sync_device_exists() &&
           isPhone &&
           sizeof(void*) == 8;
}

android::base::StringView maxVersionToFeatureString(GLESDispatchMaxVersion version) {
    switch (version) {
        case GLES_DISPATCH_MAX_VERSION_2:
            return kGLESDynamicVersion_2;
        case GLES_DISPATCH_MAX_VERSION_3_0:
            return kGLESDynamicVersion_3_0;
        case GLES_DISPATCH_MAX_VERSION_3_1:
            return kGLESDynamicVersion_3_1;
        default:
            return kGLESDynamicVersion_2;
    }
}

// OpenGL ES 3.x support involves changing the GL_VERSION string, which is
// assumed to be formatted in the following way:
// "OpenGL ES-CM 1.m <vendor-info>" or
// "OpenGL ES M.m <vendor-info>"
// where M is the major version number and m is minor version number.  If the
// GL_VERSION string doesn't reflect the maximum available version of OpenGL
// ES, many apps will not be able to detect support.  We need to mess with the
// version string in the first place since the underlying backend (whether it
// is Translator, SwiftShader, ANGLE, et al) may not advertise a GL_VERSION
// string reflecting their maximum capabilities.
std::string replaceESVersionString(const std::string& prev,
                                   android::base::StringView newver) {

    // There is no need to fiddle with the string
    // if we are in a ES 1.x context.
    // Such contexts are considered as a special case that must
    // be untouched.
    if (prev.find("ES-CM") != std::string::npos) {
        return prev;
    }

    size_t esStart = prev.find("ES ");
    size_t esEnd = prev.find(" ", esStart + 3);

    if (esStart == std::string::npos ||
        esEnd == std::string::npos) {
        // Account for out-of-spec version strings.
        fprintf(stderr, "%s: Error: invalid OpenGL ES version string %s\n",
                __func__, prev.c_str());
        return prev;
    }

    std::string res = prev.substr(0, esStart + 3);
    res += newver;
    res += prev.substr(esEnd);

    return res;
}

// If the GLES3 feature is disabled, we also want to splice out
// OpenGL extensions that should not appear in a GLES2 system.
void removeExtension(std::string& currExts, const std::string& toRemove) {
    size_t pos = currExts.find(toRemove);

    if (pos != std::string::npos)
        currExts.erase(pos, toRemove.length());
}

static EGLint rcGetGLString(EGLenum name, void* buffer, EGLint bufferSize)
{
    RenderThreadInfo *tInfo = RenderThreadInfo::get();

    // whatever we end up returning,
    // it will have a terminating \0,
    // so account for it here.
    std::string glStr;

    if (tInfo && tInfo->currContext.get()) {
        const char *str = nullptr;
        if (tInfo->currContext->version() > GLESApi_CM) {
            str = (const char *)s_gles2.glGetString(name);
        }
        else {
            str = (const char *)s_gles1.glGetString(name);
        }
        if (str) {
            glStr += str;
        }
    }

    // We add the maximum supported GL protocol number into GL_EXTENSIONS
    bool isChecksumEnabled =
        emugl_feature_is_enabled(android::featurecontrol::GLPipeChecksum);
    bool asyncSwapEnabled = shouldEnableAsyncSwap();
    bool dmaEnabled =
        emugl_feature_is_enabled(android::featurecontrol::GLDMA);
    bool glesDynamicVersionEnabled =
        emugl_feature_is_enabled(android::featurecontrol::GLESDynamicVersion);

    if (isChecksumEnabled && name == GL_EXTENSIONS) {
        glStr += ChecksumCalculatorThreadInfo::getMaxVersionString();
        glStr += " ";
    }

    if (asyncSwapEnabled && name == GL_EXTENSIONS) {
        glStr += kAsyncSwapStr;
        glStr += " ";
    }

    if (dmaEnabled && name == GL_EXTENSIONS) {
        glStr += kDmaStr;
        glStr += " ";
    }

    GLESDispatchMaxVersion maxVersion = calcMaxVersionFromDispatch();
    if (name == GL_EXTENSIONS && glesDynamicVersionEnabled) {
        glStr += maxVersionToFeatureString(maxVersion);
        glStr += " ";

        // If we have a GLES3 implementation, add the corresponding
        // GLESv2 extensions as well.
        if (maxVersion > GLES_DISPATCH_MAX_VERSION_2) {
            glStr += "GL_OES_vertex_array_object ";
        }
    }

    if (name == GL_EXTENSIONS &&
        (!glesDynamicVersionEnabled ||
         maxVersion < GLES_DISPATCH_MAX_VERSION_3_0)) {
        removeExtension(glStr, "GL_EXT_color_buffer_float ");
    }

    if (glesDynamicVersionEnabled && name == GL_VERSION) {
        GLESDispatchMaxVersion maxVersion = calcMaxVersionFromDispatch();
        switch (maxVersion) {
            case GLES_DISPATCH_MAX_VERSION_3_0:
                glStr = replaceESVersionString(glStr, "3.0");
                break;
            case GLES_DISPATCH_MAX_VERSION_3_1:
                glStr = replaceESVersionString(glStr, "3.1");
                break;
            default:
                break;
        }
    }

    int nextBufferSize = glStr.size() + 1;

    if (!buffer || nextBufferSize > bufferSize) {
        return -nextBufferSize;
    }

    snprintf((char *)buffer, nextBufferSize, "%s", glStr.c_str());
    return nextBufferSize;
}

static EGLint rcGetNumConfigs(uint32_t* p_numAttribs)
{
    int numConfigs = 0, numAttribs = 0;

    FrameBuffer::getFB()->getConfigs()->getPackInfo(&numConfigs, &numAttribs);
    if (p_numAttribs) {
        *p_numAttribs = static_cast<uint32_t>(numAttribs);
    }
    return numConfigs;
}

static EGLint rcGetConfigs(uint32_t bufSize, GLuint* buffer)
{
    GLuint bufferSize = (GLuint)bufSize;
    return FrameBuffer::getFB()->getConfigs()->packConfigs(bufferSize, buffer);
}

static EGLint rcChooseConfig(EGLint *attribs,
                             uint32_t attribs_size,
                             uint32_t *configs,
                             uint32_t configs_size)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb || attribs_size==0) {
        return 0;
    }

    return fb->getConfigs()->chooseConfig(
            attribs, (EGLint*)configs, (EGLint)configs_size);
}

static EGLint rcGetFBParam(EGLint param)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    EGLint ret = 0;

    switch(param) {
        case FB_WIDTH:
            ret = fb->getWidth();
            break;
        case FB_HEIGHT:
            ret = fb->getHeight();
            break;
        case FB_XDPI:
            ret = 72; // XXX: should be implemented
            break;
        case FB_YDPI:
            ret = 72; // XXX: should be implemented
            break;
        case FB_FPS:
            ret = 60;
            break;
        case FB_MIN_SWAP_INTERVAL:
            ret = 1; // XXX: should be implemented
            break;
        case FB_MAX_SWAP_INTERVAL:
            ret = 1; // XXX: should be implemented
            break;
        default:
            break;
    }

    return ret;
}

static uint32_t rcCreateContext(uint32_t config,
                                uint32_t share, uint32_t glVersion)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    HandleType ret = fb->createRenderContext(config, share, (GLESApi)glVersion);
    return ret;
}

static void rcDestroyContext(uint32_t context)
{
    SyncThread::destroySyncThread();

    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    fb->DestroyRenderContext(context);
}

static uint32_t rcCreateWindowSurface(uint32_t config,
                                      uint32_t width, uint32_t height)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    return fb->createWindowSurface(config, width, height);
}

static void rcDestroyWindowSurface(uint32_t windowSurface)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    fb->DestroyWindowSurface( windowSurface );
}

static uint32_t rcCreateColorBuffer(uint32_t width,
                                    uint32_t height, GLenum internalFormat)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    return fb->createColorBuffer(width, height, internalFormat,
                                 FRAMEWORK_FORMAT_GL_COMPATIBLE);
}

static uint32_t rcCreateColorBufferDMA(uint32_t width,
                                       uint32_t height, GLenum internalFormat,
                                       int frameworkFormat)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    return fb->createColorBuffer(width, height, internalFormat,
                                 (FrameworkFormat)frameworkFormat);
}

static int rcOpenColorBuffer2(uint32_t colorbuffer)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return -1;
    }
    return fb->openColorBuffer( colorbuffer );
}

static void rcOpenColorBuffer(uint32_t colorbuffer)
{
    (void) rcOpenColorBuffer2(colorbuffer);
}

static void rcCloseColorBuffer(uint32_t colorbuffer)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }
    fb->closeColorBuffer( colorbuffer );
}

static int rcFlushWindowColorBuffer(uint32_t windowSurface)
{
    GRSYNC_DPRINT("waiting for gralloc cb lock");
    GrallocSyncPostLock lock(sGrallocSync.get());
    GRSYNC_DPRINT("lock gralloc cb lock {");

    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        GRSYNC_DPRINT("unlock gralloc cb lock");
        return -1;
    }
    if (!fb->flushWindowSurfaceColorBuffer(windowSurface)) {
        GRSYNC_DPRINT("unlock gralloc cb lock }");
        return -1;
    }

    GRSYNC_DPRINT("unlock gralloc cb lock }");

    return 0;
}

// Note that even though this calls rcFlushWindowColorBuffer,
// the "Async" part is in the return type, which is void
// versus return type int for rcFlushWindowColorBuffer.
//
// The different return type, even while calling the same
// functions internally, will end up making the encoder
// and decoder use a different protocol. This is because
// the encoder generally obeys the following conventions:
//
// - The encoder will immediately send and wait for a command
//   result if the return type is not void.
// - The encoder will cache the command in a buffer and send
//   at a convenient time if the return type is void.
//
// It can also be expensive performance-wise to trigger
// sending traffic back to the guest. Generally, the more we avoid
// encoding commands that perform two-way traffic, the better.
//
// Hence, |rcFlushWindowColorBufferAsync| will avoid extra traffic;
// with return type void,
// the guest will not wait until this function returns,
// nor will it immediately send the command,
// resulting in more asynchronous behavior.
static void rcFlushWindowColorBufferAsync(uint32_t windowSurface)
{
    rcFlushWindowColorBuffer(windowSurface);
}

static void rcSetWindowColorBuffer(uint32_t windowSurface,
                                   uint32_t colorBuffer)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }
    fb->setWindowSurfaceColorBuffer(windowSurface, colorBuffer);
}

static EGLint rcMakeCurrent(uint32_t context,
                            uint32_t drawSurf, uint32_t readSurf)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return EGL_FALSE;
    }

    bool ret = fb->bindContext(context, drawSurf, readSurf);

    return (ret ? EGL_TRUE : EGL_FALSE);
}

static void rcFBPost(uint32_t colorBuffer)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    fb->post(colorBuffer);
}

static void rcFBSetSwapInterval(EGLint interval)
{
   // XXX: TBD - should be implemented
}

static void rcBindTexture(uint32_t colorBuffer)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    fb->bindColorBufferToTexture(colorBuffer);
}

static void rcBindRenderbuffer(uint32_t colorBuffer)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    fb->bindColorBufferToRenderbuffer(colorBuffer);
}

static EGLint rcColorBufferCacheFlush(uint32_t colorBuffer,
                                      EGLint postCount, int forRead)
{
   // gralloc_lock() on the guest calls rcColorBufferCacheFlush
   GRSYNC_DPRINT("waiting for gralloc cb lock");
   sGrallocSync->lockColorBufferPrepare();
   GRSYNC_DPRINT("lock gralloc cb lock {");
   return 0;
}

static void rcReadColorBuffer(uint32_t colorBuffer,
                              GLint x, GLint y,
                              GLint width, GLint height,
                              GLenum format, GLenum type, void* pixels)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return;
    }

    fb->readColorBuffer(colorBuffer, x, y, width, height, format, type, pixels);
}

static int rcUpdateColorBuffer(uint32_t colorBuffer,
                               GLint x, GLint y,
                               GLint width, GLint height,
                               GLenum format, GLenum type, void* pixels)
{
    FrameBuffer *fb = FrameBuffer::getFB();

    if (!fb) {
        GRSYNC_DPRINT("unlock gralloc cb lock");
        sGrallocSync->unlockColorBufferPrepare();
        return -1;
    }

    fb->updateColorBuffer(colorBuffer, x, y, width, height, format, type, pixels);

    GRSYNC_DPRINT("unlock gralloc cb lock");
    sGrallocSync->unlockColorBufferPrepare();

    return 0;
}

static int rcUpdateColorBufferDMA(uint32_t colorBuffer,
                                  GLint x, GLint y,
                                  GLint width, GLint height,
                                  GLenum format, GLenum type,
                                  void* pixels, uint32_t pixels_size)
{
    FrameBuffer *fb = FrameBuffer::getFB();

    if (!fb) {
        GRSYNC_DPRINT("unlock gralloc cb lock");
        sGrallocSync->unlockColorBufferPrepare();
        return -1;
    }

    fb->updateColorBuffer(colorBuffer, x, y, width, height,
                          format, type, pixels);

    GRSYNC_DPRINT("unlock gralloc cb lock");
    sGrallocSync->unlockColorBufferPrepare();
    return 0;
}

static uint32_t rcCreateClientImage(uint32_t context, EGLenum target, GLuint buffer)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    return fb->createClientImage(context, target, buffer);
}

static int rcDestroyClientImage(uint32_t image)
{
    FrameBuffer *fb = FrameBuffer::getFB();
    if (!fb) {
        return 0;
    }

    return fb->destroyClientImage(image);
}

static void rcSelectChecksumHelper(uint32_t protocol, uint32_t reserved) {
    ChecksumCalculatorThreadInfo::setVersion(protocol);
}

// |rcTriggerWait| is called from the goldfish sync
// kernel driver whenever a native fence fd is created.
// We will then need to use the host to find out
// when to signal that native fence fd. We use
// SyncThread for that.
//
// The purpose of |rcTriggerWait| is to tell which
// SyncThread which sync object / timeline to signal.
static void rcTriggerWait(uint64_t eglsync_ptr,
                          uint64_t thread_ptr,
                          uint64_t timeline) {
    FenceSync* fenceSync = (FenceSync*)(uintptr_t)eglsync_ptr;
    EGLSYNC_DPRINT("eglsync=0x%llx "
                   "fenceSync=%p "
                   "thread_ptr=0x%llx "
                   "timeline=0x%llx",
                   eglsync_ptr, fenceSync, thread_ptr, timeline);
    SyncThread* syncThread =
        SyncThread::getFromHandle(thread_ptr);
    syncThread->triggerWait(fenceSync, timeline);
}

// |rcCreateSyncKHR| implements the guest's
// |eglCreateSyncKHR| by calling the host's implementation
// of |eglCreateSyncKHR|. A SyncThread is also started
// that corresponds to the current rendering thread, for
// purposes of signaling any native fence fd's that
// get created in the guest off the sync object
// created here.
static void rcCreateSyncKHR(EGLenum type,
                            EGLint* attribs,
                            uint32_t num_attribs,
                            int destroy_when_signaled,
                            uint64_t* eglsync_out,
                            uint64_t* syncthread_out) {
    EGLSYNC_DPRINT("type=0x%x num_attribs=%d",
            type, num_attribs);

    bool hasNativeFence =
        type == EGL_SYNC_NATIVE_FENCE_ANDROID;

    // Usually we expect rcTriggerWait to be registered
    // at the beginning in rcGetRendererVersion, called
    // on init for all contexts.
    // But if we are loading from snapshot, that's not
    // guaranteed, and we need to make sure
    // rcTriggerWait is registered.
    emugl_sync_register_trigger_wait(rcTriggerWait);
    FenceSync* fenceSync = new FenceSync(hasNativeFence,
                                         destroy_when_signaled);

    // This MUST be present, or we get a deadlock effect.
    s_gles2.glFlush();

    if (syncthread_out) *syncthread_out =
        reinterpret_cast<uint64_t>(SyncThread::getSyncThread());

    if (eglsync_out) {
        uint64_t res = (uint64_t)(uintptr_t)fenceSync;
        *eglsync_out = res;
        EGLSYNC_DPRINT("send out eglsync 0x%llx", res);
    }
}

// |rcClientWaitSyncKHR| implements |eglClientWaitSyncKHR|
// on the guest through using the host's existing
// |eglClientWaitSyncKHR| implementation, which is done
// through the FenceSync object.
static EGLint rcClientWaitSyncKHR(uint64_t handle,
                                  EGLint flags,
                                  uint64_t timeout) {
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    FrameBuffer *fb = FrameBuffer::getFB();

    EGLSYNC_DPRINT("handle=0x%lx flags=0x%x timeout=%" PRIu64,
                handle, flags, timeout);

    FenceSync* fenceSync = FenceSync::getFromHandle(handle);

    if (!fenceSync) {
        EGLSYNC_DPRINT("fenceSync null, return condition satisfied");
        return EGL_CONDITION_SATISFIED_KHR;
    }

    // Sometimes a gralloc-buffer-only thread is doing stuff with sync.
    // This happens all the time with YouTube videos in the browser.
    // In this case, create a context on the host just for syncing.
    if (!tInfo->currContext) {
        uint32_t gralloc_sync_cxt, gralloc_sync_surf;
        fb->createTrivialContext(0, // There is no context to share.
                                 &gralloc_sync_cxt,
                                 &gralloc_sync_surf);
        fb->bindContext(gralloc_sync_cxt,
                        gralloc_sync_surf,
                        gralloc_sync_surf);
        // This context is then cleaned up when the render thread exits.
    }

    return fenceSync->wait(timeout);
}

static int rcDestroySyncKHR(uint64_t handle) {
    FenceSync* fenceSync = FenceSync::getFromHandle(handle);
    if (!fenceSync) return 0;
    fenceSync->decRef();
    return 0;
}

static void rcSetPuid(uint64_t puid) {
    RenderThreadInfo *tInfo = RenderThreadInfo::get();
    tInfo->m_puid = puid;
}

void initRenderControlContext(renderControl_decoder_context_t *dec)
{
    dec->rcGetRendererVersion = rcGetRendererVersion;
    dec->rcGetEGLVersion = rcGetEGLVersion;
    dec->rcQueryEGLString = rcQueryEGLString;
    dec->rcGetGLString = rcGetGLString;
    dec->rcGetNumConfigs = rcGetNumConfigs;
    dec->rcGetConfigs = rcGetConfigs;
    dec->rcChooseConfig = rcChooseConfig;
    dec->rcGetFBParam = rcGetFBParam;
    dec->rcCreateContext = rcCreateContext;
    dec->rcDestroyContext = rcDestroyContext;
    dec->rcCreateWindowSurface = rcCreateWindowSurface;
    dec->rcDestroyWindowSurface = rcDestroyWindowSurface;
    dec->rcCreateColorBuffer = rcCreateColorBuffer;
    dec->rcOpenColorBuffer = rcOpenColorBuffer;
    dec->rcCloseColorBuffer = rcCloseColorBuffer;
    dec->rcSetWindowColorBuffer = rcSetWindowColorBuffer;
    dec->rcFlushWindowColorBuffer = rcFlushWindowColorBuffer;
    dec->rcMakeCurrent = rcMakeCurrent;
    dec->rcFBPost = rcFBPost;
    dec->rcFBSetSwapInterval = rcFBSetSwapInterval;
    dec->rcBindTexture = rcBindTexture;
    dec->rcBindRenderbuffer = rcBindRenderbuffer;
    dec->rcColorBufferCacheFlush = rcColorBufferCacheFlush;
    dec->rcReadColorBuffer = rcReadColorBuffer;
    dec->rcUpdateColorBuffer = rcUpdateColorBuffer;
    dec->rcOpenColorBuffer2 = rcOpenColorBuffer2;
    dec->rcCreateClientImage = rcCreateClientImage;
    dec->rcDestroyClientImage = rcDestroyClientImage;
    dec->rcSelectChecksumHelper = rcSelectChecksumHelper;
    dec->rcCreateSyncKHR = rcCreateSyncKHR;
    dec->rcClientWaitSyncKHR = rcClientWaitSyncKHR;
    dec->rcFlushWindowColorBufferAsync = rcFlushWindowColorBufferAsync;
    dec->rcDestroySyncKHR = rcDestroySyncKHR;
    dec->rcSetPuid = rcSetPuid;
    dec->rcUpdateColorBufferDMA = rcUpdateColorBufferDMA;
    dec->rcCreateColorBufferDMA = rcCreateColorBufferDMA;
}

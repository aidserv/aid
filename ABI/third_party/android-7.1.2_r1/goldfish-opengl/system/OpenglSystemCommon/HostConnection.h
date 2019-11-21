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
#ifndef __COMMON_HOST_CONNECTION_H
#define __COMMON_HOST_CONNECTION_H

#include "IOStream.h"
#include "renderControl_enc.h"
#include "ChecksumCalculator.h"

#include <string>

class GLEncoder;
struct gl_client_context_t;
class GL2Encoder;
struct gl2_client_context_t;

// SyncImpl determines the presence of host/guest OpenGL fence sync
// capabilities. It corresponds exactly to EGL_ANDROID_native_fence_sync
// capability, but for the emulator, we need to make sure that
// OpenGL pipe protocols match, so we use a special extension name
// here.
// SYNC_IMPL_NONE means that the native fence sync capability is
// not present, and we will end up using the equivalent of glFinish
// in order to preserve buffer swapping order.
// SYNC_IMPL_NATIVE_SYNC means that we do have native fence sync
// capability, and we will use a fence fd to synchronize buffer swaps.
enum SyncImpl {
    SYNC_IMPL_NONE = 0,
    SYNC_IMPL_NATIVE_SYNC = 1
};
// Interface:
// If this GL extension string shows up, we use
// SYNC_IMPL_NATIVE_SYNC, otherwise we use SYNC_IMPL_NONE.
// This string is always updated to require the _latest_
// version of Android emulator native sync in this system image;
// otherwise, we do not use the feature.
static const char kRCNativeSync[] = "ANDROID_EMU_native_sync_v2";

// ExtendedRCEncoderContext is an extended version of renderControl_encoder_context_t
// that will be used to track SyncImpl.
class ExtendedRCEncoderContext : public renderControl_encoder_context_t {
public:
    ExtendedRCEncoderContext(IOStream *stream, ChecksumCalculator *checksumCalculator)
        : renderControl_encoder_context_t(stream, checksumCalculator) { }
    void setSyncImpl(SyncImpl syncImpl) { m_syncImpl = syncImpl; }
    bool hasNativeSync() const { return m_syncImpl == SYNC_IMPL_NATIVE_SYNC; }
private:
    SyncImpl m_syncImpl;
};

class HostConnection
{
public:
    static HostConnection *get();
    static void exit();
    ~HostConnection();

    GLEncoder *glEncoder();
    GL2Encoder *gl2Encoder();
    ExtendedRCEncoderContext *rcEncoder();
    ChecksumCalculator *checksumHelper() { return &m_checksumHelper; }

    void flush() {
        if (m_stream) {
            m_stream->flush();
        }
    }

    void setGrallocOnly(bool gralloc_only) {
        m_grallocOnly = gralloc_only;
    }

    bool isGrallocOnly() const { return m_grallocOnly; }

private:
    HostConnection();
    static gl_client_context_t  *s_getGLContext();
    static gl2_client_context_t *s_getGL2Context();

    std::string queryGLExtensions(ExtendedRCEncoderContext *rcEnc);
    // setProtocol initilizes GL communication protocol for checksums
    // should be called when m_rcEnc is created
    void setChecksumHelper(ExtendedRCEncoderContext *rcEnc);
    void queryAndSetSyncImpl(ExtendedRCEncoderContext *rcEnc);

private:
    IOStream *m_stream;
    GLEncoder   *m_glEnc;
    GL2Encoder  *m_gl2Enc;
    ExtendedRCEncoderContext *m_rcEnc;
    ChecksumCalculator m_checksumHelper;
    std::string m_glExtensions;
    bool m_grallocOnly;
};

#endif

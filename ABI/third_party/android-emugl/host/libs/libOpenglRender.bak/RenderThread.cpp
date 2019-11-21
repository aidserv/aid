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
#include "RenderThread.h"

#include "ChannelStream.h"
#include "ErrorLog.h"
#include "FrameBuffer.h"
#include "ReadBuffer.h"
#include "RenderControl.h"
#include "RendererImpl.h"
#include "RenderChannelImpl.h"
#include "RenderThreadInfo.h"

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"
#include "../../../shared/OpenglCodecCommon/ChecksumCalculatorThreadInfo.h"

#include "android/base/system/System.h"
#include "android/base/files/StreamSerializing.h"

#define EMUGL_DEBUG_LEVEL 0
#include "emugl/common/debug.h"

#include <assert.h>
// add
#include "TcpStream.h"
#include "SocketStream.h"

using android::base::AutoLock;

namespace emugl {

struct RenderThread::SnapshotObjects {
    RenderThreadInfo* threadInfo;
    ChecksumCalculator* checksumCalc;
    ChannelStream* channelStream;
    ReadBuffer* readBuffer;
};

// Start with a smaller buffer to not waste memory on a low-used render threads.
static constexpr int kStreamBufferSize = 128 * 1024;
#if 0
// 0 使用原版
// 1 使用m_IOstream
#define MY_TEST 1
RenderThread::RenderThread(RenderChannelImpl* channel,
                           android::base::Stream* loadStream)
    : emugl::Thread(android::base::ThreadFlags::MaskSignals, 2 * 1024 * 1024),
      mChannel(channel) {
    if (loadStream) {
        const bool success = loadStream->getByte();
        if (success) {
            mStream.emplace(0);
            android::base::loadStream(loadStream, &*mStream);
            mState = SnapshotState::StartLoading;
        } else {
            mFinished = true;
        }
    }
    #if MY_TEST
    m_IOstream = new ChannelStream(mChannel, RenderChannel::Buffer::kSmallSize);
    #endif
    // 初始化TCP stream;
    /* create socket;
    	m_listenSock = new TcpStream();
    	m_listenSock->listen(addrstr);
   		m_listenSock->accept();	// 每个so连接到来时 创建一个stream
   		SocketStream stream(arg1);
   		// stream->readFully();
   	*/
}
#endif // add

RenderThread::RenderThread(IOStream *myIOStream)
	: emugl::Thread(android::base::ThreadFlags::MaskSignals, 2 * 1024 * 1024)
{
	m_IOstream = myIOStream;
	printf("RenderThread m_IOStream\n");
#if 0
   	#if MY_TEST
   	#define IOSTREAM_CLIENT_EXIT_SERVER 1
   	m_listenSock = new TcpStream();
    char addrstr[256];
	printf("RenderThread.cpp ListenSocket->listen...\n");
	if (m_listenSock->listen(addrstr) < 0)
	{
		DBG("RenderThread::listen error\n");
		return;
	}
	// while(1)
	//IOStream *my_iostream = m_listenSock->accept();
	printf("RenderThread.cpp ListenSocket->accept...\n");
    SocketStream *my_iostream = m_listenSock->accept();
    if (!my_iostream) {
        DBG("Error accepting connection, aborting\n");
        return;
    }
    unsigned int clientFlags;
    if (!my_iostream->readFully(&clientFlags, sizeof(unsigned int))) {
        DBG("Error reading clientFlags\n");
        //delete my_iostream;
        return;
    }
    DBG("RenderServer: Got new stream!\n");
    if ((clientFlags & IOSTREAM_CLIENT_EXIT_SERVER) != 0) {
		// m_exiting = true;
		DBG("RenderServer FLags error\n");
		return;
    }
	m_IOstream = my_iostream;
   	#endif
#endif
}

RenderThread::~RenderThread() = default;

void RenderThread::pausePreSnapshot() {
    AutoLock lock(mLock);
    assert(mState == SnapshotState::Empty);
    //mStream.emplace();
    mState = SnapshotState::StartSaving;
    //mChannel->pausePreSnapshot();
    mCondVar.broadcastAndUnlock(&lock);
}

void RenderThread::resume() {
    AutoLock lock(mLock);
    // This function can be called for a thread from pre-snapshot loading
    // state; it doesn't need to do anything.
    if (mState == SnapshotState::Empty) {
        return;
    }
    waitForSnapshotCompletion(&lock);
    //mStream.clear();
    mState = SnapshotState::Empty;
    //mChannel->resume();
    mCondVar.broadcastAndUnlock(&lock);
}

void RenderThread::save(android::base::Stream* stream) {
    bool success;
    {
        AutoLock lock(mLock);
        assert(mState == SnapshotState::StartSaving ||
               mState == SnapshotState::InProgress ||
               mState == SnapshotState::Finished);
        waitForSnapshotCompletion(&lock);
        success = mState == SnapshotState::Finished;
    }

    if (success) {
        assert(mStream);
        stream->putByte(1);
        android::base::saveStream(stream, *mStream);
    } else {
        stream->putByte(0);
    }
}

void RenderThread::waitForSnapshotCompletion(AutoLock* lock) {
    while (mState != SnapshotState::Finished && !mFinished) {
        mCondVar.wait(lock);
    }
}

template <class OpImpl>
void RenderThread::snapshotOperation(AutoLock* lock, OpImpl&& implFunc) {
    assert(isPausedForSnapshotLocked());
    mState = SnapshotState::InProgress;
    mCondVar.broadcastAndUnlock(lock);

    implFunc();

    lock->lock();

    mState = SnapshotState::Finished;
    mCondVar.broadcast();

    // Only return after we're allowed to proceed.
    while (isPausedForSnapshotLocked()) {
        mCondVar.wait(lock);
    }
}

void RenderThread::loadImpl(AutoLock* lock, const SnapshotObjects& objects) {
    snapshotOperation(lock, [this, &objects] {
        objects.readBuffer->onLoad(&*mStream);
        objects.channelStream->load(&*mStream);
        objects.checksumCalc->load(&*mStream);
        objects.threadInfo->onLoad(&*mStream);
    });
}

void RenderThread::saveImpl(AutoLock* lock, const SnapshotObjects& objects) {
    snapshotOperation(lock, [this, &objects] {
        objects.readBuffer->onSave(&*mStream);
        objects.channelStream->save(&*mStream);
        objects.checksumCalc->save(&*mStream);
        objects.threadInfo->onSave(&*mStream);
    });
}

bool RenderThread::isPausedForSnapshotLocked() const {
    return mState != SnapshotState::Empty;
}

bool RenderThread::doSnapshotOperation(const SnapshotObjects& objects,
                                       SnapshotState state) {
    AutoLock lock(mLock);
    if (mState == state) {
        switch (state) {
            case SnapshotState::StartLoading:
                loadImpl(&lock, objects);
                return true;
            case SnapshotState::StartSaving:
                saveImpl(&lock, objects);
                return true;
            default:
                return false;
        }
    }
    return false;
}

void RenderThread::setFinished() {
    // Make sure it never happens that we wait forever for the thread to
    // save to snapshot while it was not even going to.
    AutoLock lock(mLock);
    mFinished = true;
    if (mState != SnapshotState::Empty) {
        mCondVar.broadcastAndUnlock(&lock);
    }
}
#if !MY_TEST
#define m_IOstream &stream
#endif
intptr_t RenderThread::main() {
	printf("RenderThread::main() mFinished=%d\n", mFinished);
    if (mFinished) {
        DBG("Error: fail loading a RenderThread @%p\n", this);
        return 0;
    }

    RenderThreadInfo tInfo;
    ChecksumCalculatorThreadInfo tChecksumInfo;
    ChecksumCalculator& checksumCalc = tChecksumInfo.get();
    bool needRestoreFromSnapshot = false;

    //
    // initialize decoders
    //
    tInfo.m_glDec.initGL(gles1_dispatch_get_proc_func, nullptr);
    tInfo.m_gl2Dec.initGL(gles2_dispatch_get_proc_func, nullptr);
    initRenderControlContext(&tInfo.m_rcDec);
    
    
    // IOStream &stream = m_IOstream;
	#if MY_TEST
	#else
    ChannelStream stream(mChannel, RenderChannel::Buffer::kSmallSize);
	#endif
    // m_IOstream;
    ReadBuffer readBuf(kStreamBufferSize);
#if !MY_TEST
    const SnapshotObjects snapshotObjects = {
        &tInfo, &checksumCalc, m_IOstream, &readBuf
    };

    // This is the only place where we try loading from snapshot.
    // But the context bind / restoration will be delayed after receiving
    // the first GL command.
    if (doSnapshotOperation(snapshotObjects, SnapshotState::StartLoading)) {
        DBG("Loaded RenderThread @%p from snapshot\n", this);
        needRestoreFromSnapshot = true;
    } else 
#endif
#if !MY_TEST
	{
        // Not loading from a snapshot: continue regular startup, read
        // the |flags|.
        uint32_t flags = 0;
        #if MY_TEST
        //while (m_IOstream->read(&flags, sizeof(flags)) != sizeof(flags)) {
        while (m_IOstream->read(&flags, sizeof(flags)) != sizeof(flags)) {
        
        //printf("read flags=%d\n", flags);
        #else
        while (stream.read(&flags, sizeof(flags)) != sizeof(flags)) {
        #endif
            // Stream read may fail because of a pending snapshot.
            #if MY_TEST
            setFinished();
            #else
            if (!doSnapshotOperation(snapshotObjects, SnapshotState::StartSaving)) {
                setFinished();
                DBG("Exited a RenderThread @%p early\n", this);
                return 0;
            }
            #endif
        }

        // |flags| used to mean something, now they're not used.
        (void)flags;
    }
#endif
    printf("Render while out\n");
    int stats_totalBytes = 0;
    auto stats_t0 = android::base::System::get()->getHighResTimeUs() / 1000;

    //
    // open dump file if RENDER_DUMP_DIR is defined
    //
    const char* dump_dir = getenv("RENDERER_DUMP_DIR");
    FILE* dumpFP = nullptr;
    if (dump_dir) {
        size_t bsize = strlen(dump_dir) + 32;
        char* fname = new char[bsize];
        snprintf(fname, bsize, "%s/stream_%p", dump_dir, this);
        dumpFP = fopen(fname, "wb");
        if (!dumpFP) {
            fprintf(stderr, "Warning: stream dump failed to open file %s\n",
                    fname);
        }
        delete[] fname;
    }

    while (1) {
        // Let's make sure we read enough data for at least some processing.
        int packetSize;
        if (readBuf.validData() >= 8) {
            // We know that packet size is the second int32_t from the start.
            packetSize = *(const int32_t*)(readBuf.buf() + 4);
        } else {
            // Read enough data to at least be able to get the packet size next
            // time.
            packetSize = 8;
        }

        int stat = 0;
        if (packetSize > (int)readBuf.validData()) {
            stat = readBuf.getData(m_IOstream, packetSize);
            if (stat <= 0) {
            #if MY_TEST
            break;	// 得加一个退出不然死循环
            #else
                if (doSnapshotOperation(snapshotObjects, SnapshotState::StartSaving)) {
                    continue;
                } else {
                    D("Warning: render thread could not read data from stream");
                    break;
                }
            #endif
            } else if (needRestoreFromSnapshot) {
                // We just loaded from a snapshot, need to initialize / bind
                // the contexts.
                needRestoreFromSnapshot = false;
                HandleType ctx = tInfo.currContext ? tInfo.currContext->getHndl()
                        : 0;
                HandleType draw = tInfo.currDrawSurf ? tInfo.currDrawSurf->getHndl()
                        : 0;
                HandleType read = tInfo.currReadSurf ? tInfo.currReadSurf->getHndl()
                        : 0;
                FrameBuffer::getFB()->bindContext(ctx, draw, read);
            }
        }

        DD("render thread read %d bytes, op %d, packet size %d",
           (int)readBuf.validData(), *(int32_t*)readBuf.buf(),
           *(int32_t*)(readBuf.buf() + 4));

        //
        // log received bandwidth statistics
        //
        stats_totalBytes += readBuf.validData();
        auto dt = android::base::System::get()->getHighResTimeUs() / 1000 - stats_t0;
        if (dt > 1000) {
            // float dts = (float)dt / 1000.0f;
            // printf("Used Bandwidth %5.3f MB/s\n", ((float)stats_totalBytes /
            // dts) / (1024.0f*1024.0f));
            stats_totalBytes = 0;
            stats_t0 = android::base::System::get()->getHighResTimeUs() / 1000;
        }

        //
        // dump stream to file if needed
        //
        if (dumpFP) {
            int skip = readBuf.validData() - stat;
            fwrite(readBuf.buf() + skip, 1, readBuf.validData() - skip, dumpFP);
            fflush(dumpFP);
        }

        bool progress;
        do {
            progress = false;

            // try to process some of the command buffer using the GLESv1
            // decoder
            //
            // DRIVER WORKAROUND:
            // On Linux with NVIDIA GPU's at least, we need to avoid performing
            // GLES ops while someone else holds the FrameBuffer write lock.
            //
            // To be more specific, on Linux with NVIDIA Quadro K2200 v361.xx,
            // we get a segfault in the NVIDIA driver when glTexSubImage2D
            // is called at the same time as glXMake(Context)Current.
            //
            // To fix, this driver workaround avoids calling
            // any sort of GLES call when we are creating/destroying EGL
            // contexts.
            FrameBuffer::getFB()->lockContextStructureRead();
            size_t last = tInfo.m_glDec.decode(
                    readBuf.buf(), readBuf.validData(), m_IOstream, &checksumCalc);
            if (last > 0) {
                progress = true;
                readBuf.consume(last);
            }

            //
            // try to process some of the command buffer using the GLESv2
            // decoder
            //
            last = tInfo.m_gl2Dec.decode(readBuf.buf(), readBuf.validData(),
                                         m_IOstream, &checksumCalc);
            FrameBuffer::getFB()->unlockContextStructureRead();

            if (last > 0) {
                progress = true;
                readBuf.consume(last);
            }

            //
            // try to process some of the command buffer using the
            // renderControl decoder
            //
            last = tInfo.m_rcDec.decode(readBuf.buf(), readBuf.validData(),
                                        m_IOstream, &checksumCalc);
            if (last > 0) {
                readBuf.consume(last);
                progress = true;
            }
        } while (progress);
    }

    setFinished();

    if (dumpFP) {
        fclose(dumpFP);
    }

    // Don't check for snapshots here: if we're already exiting then snapshot
    // should not contain this thread information at all.

    // exit sync thread, if any.
    SyncThread::destroySyncThread();

    // Release references to the current thread's context/surfaces if any
    FrameBuffer::getFB()->bindContext(0, 0, 0);
    if (tInfo.currContext || tInfo.currDrawSurf || tInfo.currReadSurf) {
        fprintf(stderr,
                "ERROR: RenderThread exiting with current context/surfaces\n");
    }

    FrameBuffer::getFB()->drainWindowSurface();
    FrameBuffer::getFB()->drainRenderContext();

    DBG("Exited a RenderThread @%p\n", this);

    return 0;
}
#if !MY_TEST
#undef m_IOstream
#endif 
}  // namespace emugl

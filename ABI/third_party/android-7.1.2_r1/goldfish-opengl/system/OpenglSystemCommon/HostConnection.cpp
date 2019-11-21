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
#include "HostConnection.h"

#include "GLEncoder.h"
#include "GL2Encoder.h"
#include "ProcessPipe.h"
#include "QemuPipeStream.h"
#include "TcpStream.h"
#include "ThreadInfo.h"

#include <cutils/log.h>

#define STREAM_BUFFER_SIZE  (4*1024*1024)
#define STREAM_PORT_NUM     22468

/* Set to 1 to use a QEMU pipe, or 0 for a TCP connection */
#define  USE_QEMU_PIPE  1

HostConnection::HostConnection() :
    m_stream(NULL),
    m_glEnc(NULL),
    m_gl2Enc(NULL),
    m_rcEnc(NULL),
    m_checksumHelper(),
    m_glExtensions(),
    m_grallocOnly(true)
{
}

HostConnection::~HostConnection()
{
    delete m_stream;
    delete m_glEnc;
    delete m_gl2Enc;
    delete m_rcEnc;
}

HostConnection *HostConnection::get() {

    /* TODO: Make this configurable with a system property */
    const int useQemuPipe = USE_QEMU_PIPE;

    // Get thread info
    EGLThreadInfo *tinfo = getEGLThreadInfo();
    if (!tinfo) {
        return NULL;
    }

    if (tinfo->hostConn == NULL) {
        HostConnection *con = new HostConnection();
        if (NULL == con) {
            return NULL;
        }

        if (useQemuPipe) {
            QemuPipeStream *stream = new QemuPipeStream(STREAM_BUFFER_SIZE);
            if (!stream) {
                ALOGE("Failed to create QemuPipeStream for host connection!!!\n");
                delete con;
                return NULL;
            }
            if (stream->connect() < 0) {
                ALOGE("Failed to connect to host (QemuPipeStream)!!!\n");
                delete stream;
                delete con;
                return NULL;
            }
            con->m_stream = stream;
        }
        else /* !useQemuPipe */
        {
            TcpStream *stream = new TcpStream(STREAM_BUFFER_SIZE);
            if (!stream) {
                ALOGE("Failed to create TcpStream for host connection!!!\n");
                delete con;
                return NULL;
            }

            if (stream->connect("10.0.2.2", STREAM_PORT_NUM) < 0) {
                ALOGE("Failed to connect to host (TcpStream)!!!\n");
                delete stream;
                delete con;
                return NULL;
            }
            con->m_stream = stream;
        }

        // send zero 'clientFlags' to the host.
        unsigned int *pClientFlags =
                (unsigned int *)con->m_stream->allocBuffer(sizeof(unsigned int));
        *pClientFlags = 0;
        con->m_stream->commitBuffer(sizeof(unsigned int));

        ALOGD("HostConnection::get() New Host Connection established %p, tid %d\n", con, gettid());
        tinfo->hostConn = con;
    }

    return tinfo->hostConn;
}

void HostConnection::exit() {
    EGLThreadInfo *tinfo = getEGLThreadInfo();
    if (!tinfo) {
        return;
    }

    if (tinfo->hostConn) {
        delete tinfo->hostConn;
        tinfo->hostConn = NULL;
    }
}



GLEncoder *HostConnection::glEncoder()
{
    if (!m_glEnc) {
        m_glEnc = new GLEncoder(m_stream, checksumHelper());
        DBG("HostConnection::glEncoder new encoder %p, tid %d", m_glEnc, gettid());
        m_glEnc->setContextAccessor(s_getGLContext);
    }
    return m_glEnc;
}

GL2Encoder *HostConnection::gl2Encoder()
{
    if (!m_gl2Enc) {
        m_gl2Enc = new GL2Encoder(m_stream, checksumHelper());
        DBG("HostConnection::gl2Encoder new encoder %p, tid %d", m_gl2Enc, gettid());
        m_gl2Enc->setContextAccessor(s_getGL2Context);
    }
    return m_gl2Enc;
}

ExtendedRCEncoderContext *HostConnection::rcEncoder()
{
    if (!m_rcEnc) {
        m_rcEnc = new ExtendedRCEncoderContext(m_stream, checksumHelper());
        setChecksumHelper(m_rcEnc);
        queryAndSetSyncImpl(m_rcEnc);
        processPipeInit(m_rcEnc);
    }
    return m_rcEnc;
}

gl_client_context_t *HostConnection::s_getGLContext()
{
    EGLThreadInfo *ti = getEGLThreadInfo();
    if (ti->hostConn) {
        return ti->hostConn->m_glEnc;
    }
    return NULL;
}

gl2_client_context_t *HostConnection::s_getGL2Context()
{
    EGLThreadInfo *ti = getEGLThreadInfo();
    if (ti->hostConn) {
        return ti->hostConn->m_gl2Enc;
    }
    return NULL;
}

std::string HostConnection::queryGLExtensions(ExtendedRCEncoderContext *rcEnc) {
    if (m_glExtensions.size() > 0) return m_glExtensions;

    std::string extensions_buffer;
    int extensionSize = rcEnc->rcGetGLString(rcEnc, GL_EXTENSIONS, NULL, 0);
    if (extensionSize < 0) {

        extensions_buffer.resize(-extensionSize);
        extensionSize = rcEnc->rcGetGLString(rcEnc, GL_EXTENSIONS,
                                             &extensions_buffer[0], -extensionSize);

        if (extensionSize <= 0) {
            return std::string();
        }

        m_glExtensions += extensions_buffer;

        return m_glExtensions;
    }

    return std::string();
}

void HostConnection::setChecksumHelper(ExtendedRCEncoderContext *rcEnc) {
    std::string glExtensions = queryGLExtensions(rcEnc);
    // check the host supported version
    uint32_t checksumVersion = 0;
    const char* checksumPrefix = ChecksumCalculator::getMaxVersionStrPrefix();
    const char* glProtocolStr = strstr(glExtensions.c_str(), checksumPrefix);
    if (glProtocolStr) {
        uint32_t maxVersion = ChecksumCalculator::getMaxVersion();
        sscanf(glProtocolStr+strlen(checksumPrefix), "%d", &checksumVersion);
        if (maxVersion < checksumVersion) {
            checksumVersion = maxVersion;
        }
        // The ordering of the following two commands matters!
        // Must tell the host first before setting it in the guest
        rcEnc->rcSelectChecksumHelper(rcEnc, checksumVersion, 0);
        m_checksumHelper.setVersion(checksumVersion);
    }
}

void HostConnection::queryAndSetSyncImpl(ExtendedRCEncoderContext *rcEnc) {
    std::string glExtensions = queryGLExtensions(rcEnc);
#if PLATFORM_SDK_VERSION <= 16
    rcEnc->setSyncImpl(SYNC_IMPL_NONE);
#else
    if (glExtensions.find(kRCNativeSync) != std::string::npos) {
        rcEnc->setSyncImpl(SYNC_IMPL_NATIVE_SYNC);
    } else {
        rcEnc->setSyncImpl(SYNC_IMPL_NONE);
    }
#endif
}

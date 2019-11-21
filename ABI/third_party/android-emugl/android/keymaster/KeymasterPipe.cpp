// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/keymaster/KeymasterPipe.h"
#include "android/keymaster/Keymaster.h"
#include "android/utils/debug.h"

#include <assert.h>

#ifdef DEBUG
#  include <stdio.h>
#  define  D(...)   ( fprintf( stderr, __VA_ARGS__ ) )
#else
#  define  D(...)   ((void)0)
#endif

template <class Decoded_t>
static Decoded_t unpackAndAdvance(uint8_t** data) {
    Decoded_t ret;
    memcpy(&ret, *data, sizeof(ret));
    *data = *data + sizeof(Decoded_t);
    return ret;
}

// Read param from src.
// Return a pointer for the param
static void* readGenParam(uint8_t* src,
    const keymaster_keypair_t type) {
    switch (type) {
        case TYPE_DSA: {
            keymaster_dsa_keygen_params_t* dsaParams =
                    new keymaster_dsa_keygen_params_t();
            dsaParams->key_size = unpackAndAdvance<uint32_t>(&src);
            dsaParams->generator_len = unpackAndAdvance<uint32_t>(&src);
            dsaParams->prime_p_len = unpackAndAdvance<uint32_t>(&src);
            dsaParams->prime_q_len = unpackAndAdvance<uint32_t>(&src);
            dsaParams->generator = src;
            src += dsaParams->generator_len;
            dsaParams->prime_p = src;
            src += dsaParams->prime_p_len;
            dsaParams->prime_q = src;
            src += dsaParams->prime_q_len;
            return (void*)dsaParams;
            break;
        }
        case TYPE_EC: {
            keymaster_ec_keygen_params_t* ecParams =
                    new keymaster_ec_keygen_params_t();
            ecParams->field_size = unpackAndAdvance<uint32_t>(&src);
            return (void*)ecParams;
            break;
        }
        case TYPE_RSA: {
            keymaster_rsa_keygen_params_t* rsaParams =
                new keymaster_rsa_keygen_params_t();
            rsaParams->modulus_size = unpackAndAdvance<uint32_t>(&src);
            rsaParams->public_exponent = unpackAndAdvance<uint64_t>(&src);
            return (void*)rsaParams;
            break;
        }
        default:
            D("Unsupported key type %d\n", type);
            return nullptr;
    }
}

static void releaseGenKeyParam(const keymaster_keypair_t type,
        void** param) {
    switch (type) {
        case TYPE_DSA: {
            keymaster_dsa_keygen_params_t* dsaParams =
                    (keymaster_dsa_keygen_params_t*)*param;
            // dsaParams does not have ownership for its pointer fields
            // do not delete them
            delete dsaParams;
            break;
        }
        case TYPE_EC: {
            keymaster_ec_keygen_params_t* ecParams =
                    (keymaster_ec_keygen_params_t*)*param;
            delete ecParams;
            break;
        }
        case TYPE_RSA: {
            keymaster_rsa_keygen_params_t* rsaParams =
                    (keymaster_rsa_keygen_params_t*)*param;
            delete rsaParams;
            break;
        }
        default:
            D("Unsupported key type %d\n", type);
    }
    *param = nullptr;
}

// Read param from src.
// Return a pointer for the param
static void* readSignParam(uint8_t* src,
    const keymaster_keypair_t type) {
    switch (type) {
        case TYPE_DSA: {
            keymaster_dsa_sign_params_t* dsaParams =
                new keymaster_dsa_sign_params_t();
            dsaParams->digest_type =
                    unpackAndAdvance<keymaster_digest_algorithm_t>(&src);
            return (void*)dsaParams;
        }
        case TYPE_EC: {
            keymaster_ec_sign_params_t* ecParams =
                new keymaster_ec_sign_params_t();
            ecParams->digest_type =
                    unpackAndAdvance<keymaster_digest_algorithm_t>(&src);
            return (void*)ecParams;
        }
        case TYPE_RSA: {
            keymaster_rsa_sign_params_t* rsaParams =
                new keymaster_rsa_sign_params_t();
            rsaParams->digest_type =
                    unpackAndAdvance<keymaster_digest_algorithm_t>(&src);
            rsaParams->padding_type =
                    unpackAndAdvance<keymaster_rsa_padding_t>(&src);
            return (void*)rsaParams;
        }
        default:
            D("Unsupported key type %d\n", type);
            return nullptr;
    }
}

static void releaseSignKeyParam(const keymaster_keypair_t type,
        void** param) {
    switch (type) {
        case TYPE_DSA: {
            keymaster_dsa_sign_params_t* dsaParams =
                    (keymaster_dsa_sign_params_t*)*param;
            delete dsaParams;
            break;
        }
        case TYPE_EC: {
            keymaster_ec_sign_params_t* ecParams =
                    (keymaster_ec_sign_params_t*)*param;
            delete ecParams;
            break;
        }
        case TYPE_RSA: {
            keymaster_rsa_sign_params_t* rsaParams =
                    (keymaster_rsa_sign_params_t*)*param;
            delete rsaParams;
            break;
        }
        default:
            D("Unsupported key type %d\n", type);
    }
    *param = nullptr;
}

namespace android {

KeymasterPipe::KeymasterPipe(void* hwPipe, Service* service) :
                            AndroidPipe(hwPipe, service) {
    D("register new keymaster pipe\n");
}

unsigned KeymasterPipe::onGuestPoll() const {
    switch (mState) {
        case State::WaitingForGuestSend:
        case State::WaitingForGuestSendPartial:
            return PIPE_POLL_OUT;
        case State::WaitingForGuestRecv:
        case State::WaitingForGuestRecvPartial:
            return PIPE_POLL_IN;
    }
    return 0;
}

int KeymasterPipe::onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
    int bytesRecv = 0;
    while (numBuffers > 0 && mGuestRecvBufferHead != mGuestRecvBuffer.size()) {
        size_t dataSize = mGuestRecvBuffer.size() - mGuestRecvBufferHead;
        if (dataSize > buffers->size) {
            dataSize = buffers->size;
        }
        memcpy(buffers->data, &mGuestRecvBuffer[mGuestRecvBufferHead],
                dataSize);
        bytesRecv += dataSize;
        mGuestRecvBufferHead += dataSize;
        buffers ++;
        numBuffers --;
    }
    if (mGuestRecvBufferHead == mGuestRecvBuffer.size()) {
        mGuestRecvBuffer.clear();
        mState = State::WaitingForGuestSend;
    } else {
        mState = State::WaitingForGuestRecvPartial;
    }
    return bytesRecv;
}

int KeymasterPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                        int numBuffers) {
    int bytesSent = 0;
    while (numBuffers > 0) {
        const uint8_t* data = nullptr;
        size_t dataSize = 0;
        D("keymaster receive buffer size %lu\n", buffers->size);
        switch (mState) {
            case State::WaitingForGuestSend: {
                if (buffers->size < 8) {
                    derror("ERROR: KeymasterPipe: received incomplete command,"
                        " expect size 8, received size %lu\n",
                        buffers->size);
                    return PIPE_ERROR_INVAL;
                }
                uint64_t commandLength;
                memcpy(&commandLength, buffers->data, sizeof(commandLength));
                dataSize = buffers->size - sizeof(commandLength);
                D("keymaster new command, length %lu\n", commandLength);
                mCommandBufferOffset = 0;
                data = buffers->data + sizeof(uint64_t);
                mState = State::WaitingForGuestSendPartial;
                if (commandLength > mCommandBuffer.size()) {
                    mCommandBuffer.resize(commandLength);
                }
                break;
            }
            case State::WaitingForGuestSendPartial:
                dataSize = buffers->size;
                data = buffers->data;
                break;
            default: // this is not expected
                derror("ERROR: KeymasterPipe: received command"
                    "while pending for send\n");
                return PIPE_ERROR_INVAL;
        }
        bytesSent += buffers->size;
        // we are not expecting more than one command at a time
        assert(mCommandBufferOffset + dataSize <= mCommandBuffer.size());
        memcpy(&mCommandBuffer[0] + mCommandBufferOffset, data, dataSize);
        mCommandBufferOffset += dataSize;
        buffers++;
        numBuffers--;
        mState = State::WaitingForGuestSendPartial;
    }
    if (mCommandBufferOffset == mCommandBuffer.size()) {
        // decode and execute the command
        decodeAndExecute();
        mCommandBuffer.clear();
    }
    return bytesSent;
}

void KeymasterPipe::addToRecvBuffer(const uint8_t* data, uint32_t len) {
    mGuestRecvBuffer.insert(mGuestRecvBuffer.end(), data, data + len);
}

void KeymasterPipe::decodeAndExecute() {
    enum {
        GenerateKeypair,
        ImportKeypair,
        GetKeypairPublic,
        SignData,
        VerifyData,
    };
    mGuestRecvBufferHead = 0;
    uint8_t* data = &mCommandBuffer[0];
    int32_t cmd = unpackAndAdvance<int32_t>(&data);
    switch (cmd) {
        case GenerateKeypair:
            {
                D("%s: generating key\n", __FUNCTION__);
                // input parameters
                keymaster_keypair_t keyType =
                        (keymaster_keypair_t)unpackAndAdvance<int32_t>(&data);
                uint32_t keyParamsLen = unpackAndAdvance<uint32_t>(&data);
                void* keyParams = readGenParam(data, keyType);
                data += keyParamsLen;
                // output parameters
                std::vector<uint8_t> keyBlob;
                int32_t ret = Keymaster::generateKeypair(keyType,
                        keyParams, &keyBlob);
                uint32_t keyBlobLength = keyBlob.size();
                addToRecvBuffer((uint8_t*)&keyBlobLength,
                        sizeof(keyBlobLength));
                if (keyBlobLength) {
                    addToRecvBuffer(keyBlob.data(), keyBlobLength);
                }
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
                releaseGenKeyParam(keyType, &keyParams);
            }
            break;
        case ImportKeypair:
            {
                D("%s: importing key pair\n", __FUNCTION__);
                // input parameters
                uint32_t keyLen = unpackAndAdvance<uint32_t>(&data);
                const uint8_t* key = (uint8_t*)data;
                data += keyLen;
                // output parameters
                std::vector<uint8_t> keyBlob;
                int32_t ret = Keymaster::importKeypair(key, keyLen,
                        &keyBlob);
                uint32_t keyBlobLength = keyBlob.size();
                addToRecvBuffer((uint8_t*)&keyBlobLength,
                        sizeof(keyBlobLength));
                addToRecvBuffer(keyBlob.data(), keyBlobLength);
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
            }
            break;
        case GetKeypairPublic:
            {
                D("%s: getting key pair\n", __FUNCTION__);
                // input parameters
                uint32_t keyBlobLen = unpackAndAdvance<uint32_t>(&data);
                const uint8_t* keyBlob = (uint8_t*)data;
                data += keyBlobLen;
                // output parameters
                std::vector<uint8_t> x509Data;
                int32_t ret = Keymaster::getKeypairPublic(keyBlob,
                        keyBlobLen, &x509Data);
                uint32_t x509DataLen = x509Data.size();
                addToRecvBuffer((uint8_t*)&x509DataLen,
                        sizeof(x509DataLen));
                if (x509DataLen) {
                    addToRecvBuffer(x509Data.data(), x509DataLen);
                }
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
            }
            break;
        case SignData:
            {
                D("%s: %d signing data\n", __FUNCTION__, __LINE__);
                // input parameters
                keymaster_keypair_t keyType =
                        (keymaster_keypair_t)unpackAndAdvance<int32_t>(&data);
                uint32_t keyParamsLen = unpackAndAdvance<uint32_t>(&data);
                void* keyParams = readSignParam(data, keyType);
                data += keyParamsLen;
                uint32_t keyBlobLen = unpackAndAdvance<uint32_t>(&data);
                const uint8_t* keyBlob = (uint8_t*)data;
                data += keyBlobLen;
                uint32_t rawDataLen = unpackAndAdvance<uint32_t>(&data);
                const uint8_t* rawData = (uint8_t*)data;
                data += rawDataLen;
                // output parameters
                std::vector<uint8_t> signedData;
                int32_t ret = Keymaster::signData(keyParams,
                        keyBlob, keyBlobLen,
                        rawData, rawDataLen,
                        &signedData);
                uint32_t signedDataLen = signedData.size();
                addToRecvBuffer((uint8_t*)&signedDataLen,
                        sizeof(signedDataLen));
                if (signedDataLen) {
                    addToRecvBuffer(signedData.data(), signedDataLen);
                }
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
                releaseSignKeyParam(keyType, &keyParams);
            }
            break;
        case VerifyData:
            {
                D("%s: verifying data\n", __FUNCTION__);
                // input parameters
                keymaster_keypair_t keyType =
                        (keymaster_keypair_t)unpackAndAdvance<int32_t>(&data);
                uint32_t keyParamsLen = unpackAndAdvance<uint32_t>(&data);
                void* keyParams = readSignParam(data, keyType);
                data += keyParamsLen;
                uint32_t keyBlobLen = unpackAndAdvance<uint32_t>(&data);
                const uint8_t* keyBlob = (uint8_t*)data;
                data += keyBlobLen;
                uint32_t signedDataLen = unpackAndAdvance<uint32_t>(&data);
                const uint8_t* signedData = (uint8_t*)data;
                data += signedDataLen;
                uint32_t signatureLen = unpackAndAdvance<uint32_t>(&data);
                const uint8_t* signature = (uint8_t*)data;
                data += signatureLen;
                // output parameters
                int32_t ret = Keymaster::verifyData(keyParams,
                        keyBlob, keyBlobLen,
                        signedData, signedDataLen,
                        signature, signatureLen);
                addToRecvBuffer((uint8_t*)&ret, sizeof(ret));
                releaseSignKeyParam(keyType, &keyParams);
            }
            break;
        default:
            derror("ERROR: KeymasterPipe: received bad command %d,"
                    "Shutting down keymaster pipe\n",
                    cmd);
            closeFromHost();
            return;
    }
    // Tell the guest we are ready to send the return value
    mState = State::WaitingForGuestRecv;
}

void registerKeymasterPipeService() {
    android::AndroidPipe::Service::add(new KeymasterPipe::Service());
}
} // namespace android

extern "C" void android_init_keymaster_pipe(void) {
    android::registerKeymasterPipeService();
}
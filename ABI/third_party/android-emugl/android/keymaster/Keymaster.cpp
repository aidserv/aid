/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/keymaster/keymaster_common.h"
#include "android/keymaster/Keymaster.h"

#include <errno.h>
#include <string.h>
#include <stdint.h>

#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/x509.h>

#include <memory>

#ifdef DEBUG
#  include <stdio.h>
#  define  D(...)   ( fprintf( stderr, __VA_ARGS__ ), fprintf(stderr, "\n") )
#else
#  define  D(...)   ((void)0)
#endif

struct BIGNUM_Delete {
    void operator()(BIGNUM* p) const { BN_free(p); }
};
typedef std::unique_ptr<BIGNUM, BIGNUM_Delete> Unique_BIGNUM;

struct EVP_PKEY_Delete {
    void operator()(EVP_PKEY* p) const { EVP_PKEY_free(p); }
};
typedef std::unique_ptr<EVP_PKEY, EVP_PKEY_Delete> Unique_EVP_PKEY;

struct PKCS8_PRIV_KEY_INFO_Delete {
    void operator()(PKCS8_PRIV_KEY_INFO* p) const { PKCS8_PRIV_KEY_INFO_free(p); }
};
typedef std::unique_ptr<PKCS8_PRIV_KEY_INFO, PKCS8_PRIV_KEY_INFO_Delete> Unique_PKCS8_PRIV_KEY_INFO;

struct DSA_Delete {
    void operator()(DSA* p) const { DSA_free(p); }
};
typedef std::unique_ptr<DSA, DSA_Delete> Unique_DSA;

struct EC_KEY_Delete {
    void operator()(EC_KEY* p) const { EC_KEY_free(p); }
};
typedef std::unique_ptr<EC_KEY, EC_KEY_Delete> Unique_EC_KEY;

struct EC_GROUP_Delete {
    void operator()(EC_GROUP* p) const { EC_GROUP_free(p); }
};
typedef std::unique_ptr<EC_GROUP, EC_GROUP_Delete> Unique_EC_GROUP;

struct RSA_Delete {
    void operator()(RSA* p) const { RSA_free(p); }
};
typedef std::unique_ptr<RSA, RSA_Delete> Unique_RSA;

struct Malloc_Free {
    void operator()(void* p) const { free(p); }
};

/**
 * Many OpenSSL APIs take ownership of an argument on success but
 * don't free the argument on failure. This means we need to tell our
 * scoped pointers when we've transferred ownership, without
 * triggering a warning by not using the result of release().
 */
template <typename T, typename Delete_T>
inline void release_because_ownership_transferred(
        std::unique_ptr<T, Delete_T>& p) {
    T* val __attribute__((unused)) = p.release();
}

/*
 * Checks this thread's OpenSSL error queue and logs if
 * necessary.
 */
static void logOpenSSLError(const char* location) {
    int error = ERR_get_error();

    if (error != 0) {
        char message[256];
        ERR_error_string_n(error, message, sizeof(message));
        D("OpenSSL error in %s %d: %s", location, error, message);
    }

    ERR_clear_error();
    ERR_remove_thread_state(NULL);
}

static int wrap_key(EVP_PKEY* pkey, int type, std::vector<uint8_t>* keyBlob) {
    /*
     * Find the length of each size. Public key is not needed anymore
     * but must be kept for alignment purposes.
     */
    int publicLen = 0;
    int privateLen = i2d_PrivateKey(pkey, NULL);

    if (privateLen <= 0) {
        D("private key size was too big");
        return -1;
    }

    /* int type + int size + private key data + int size + public key data */
    size_t keyBlobLength = sizeof(type) + sizeof(publicLen) + privateLen +
                     sizeof(privateLen) + publicLen;

    keyBlob->resize(keyBlobLength);
    if (keyBlob->empty()) {
        D("could not allocate memory for key blob");
        return -1;
    }
    unsigned char* p = keyBlob->data();

    /* Write key type to allocated buffer */
    for (int i = sizeof(type) - 1; i >= 0; i--) {
        *p++ = (type >> (8 * i)) & 0xFF;
    }

    /* Write public key to allocated buffer */
    for (int i = sizeof(publicLen) - 1; i >= 0; i--) {
        *p++ = (publicLen >> (8 * i)) & 0xFF;
    }

    /* Write private key to allocated buffer */
    for (int i = sizeof(privateLen) - 1; i >= 0; i--) {
        *p++ = (privateLen >> (8 * i)) & 0xFF;
    }
    if (i2d_PrivateKey(pkey, &p) != privateLen) {
        logOpenSSLError("wrap_key");
        return -1;
    }

    return 0;
}

static EVP_PKEY* unwrap_key(const uint8_t* keyBlob, const size_t keyBlobLength) {
    long publicLen = 0;
    long privateLen = 0;
    const uint8_t* p = keyBlob;
    const uint8_t* const end = keyBlob + keyBlobLength;

    if (keyBlob == NULL) {
        D("supplied key blob was NULL");
        return NULL;
    }

    int32_t type = 0;
    if (keyBlobLength < (sizeof(type) + sizeof(publicLen) + 1 +
                         sizeof(privateLen) + 1)) {
        D("key blob appears to be truncated");
        return NULL;
    }

    for (size_t i = 0; i < sizeof(type); i++) {
        type = (type << 8) | *p++;
    }

    for (size_t i = 0; i < sizeof(type); i++) {
        publicLen = (publicLen << 8) | *p++;
    }
    if (p + publicLen > end) {
        D("public key length encoding error: size=%ld, end=%td", publicLen,
                end - p);
        return NULL;
    }

    p += publicLen;
    if (end - p < 2) {
        D("private key truncated");
        return NULL;
    }
    for (size_t i = 0; i < sizeof(type); i++) {
        privateLen = (privateLen << 8) | *p++;
    }
    if (p + privateLen > end) {
        D("private key length encoding error: size=%ld, end=%td",
                privateLen, end - p);
        return NULL;
    }

    Unique_EVP_PKEY pkey(EVP_PKEY_new());
    if (pkey.get() == NULL) {
        logOpenSSLError("unwrap_key");
        return NULL;
    }
    EVP_PKEY* tmp = pkey.get();

    if (d2i_PrivateKey(type, &tmp, &p, privateLen) == NULL) {
        logOpenSSLError("unwrap_key");
        return NULL;
    }

    return pkey.release();
}

static int generate_dsa_keypair(EVP_PKEY* pkey,
        const keymaster_dsa_keygen_params_t* dsa_params) {
    if (dsa_params->key_size < 512) {
        D("Requested DSA key size is too small (<512)");
        return -1;
    }

    Unique_DSA dsa(DSA_new());

    if (dsa_params->generator_len == 0 || dsa_params->prime_p_len == 0 ||
        dsa_params->prime_q_len == 0 || dsa_params->generator == NULL ||
        dsa_params->prime_p == NULL || dsa_params->prime_q == NULL) {
        if (DSA_generate_parameters_ex(dsa.get(), dsa_params->key_size, NULL, 0,
                NULL, NULL, NULL) != 1) {
            logOpenSSLError("generate_dsa_keypair");
            return -1;
        }
    } else {
        dsa->g = BN_bin2bn(dsa_params->generator, dsa_params->generator_len,
                NULL);
        if (dsa->g == NULL) {
            logOpenSSLError("generate_dsa_keypair");
            return -1;
        }

        dsa->p = BN_bin2bn(dsa_params->prime_p, dsa_params->prime_p_len, NULL);
        if (dsa->p == NULL) {
            logOpenSSLError("generate_dsa_keypair");
            return -1;
        }

        dsa->q = BN_bin2bn(dsa_params->prime_q, dsa_params->prime_q_len, NULL);
        if (dsa->q == NULL) {
            logOpenSSLError("generate_dsa_keypair");
            return -1;
        }
    }

    if (DSA_generate_key(dsa.get()) != 1) {
        logOpenSSLError("generate_dsa_keypair");
        return -1;
    }

    if (EVP_PKEY_assign_DSA(pkey, dsa.get()) == 0) {
        logOpenSSLError("generate_dsa_keypair");
        return -1;
    }
    release_because_ownership_transferred(dsa);

    return 0;
}

static int generate_ec_keypair(EVP_PKEY* pkey,
        const keymaster_ec_keygen_params_t* ec_params) {
    Unique_EC_GROUP group;
    switch (ec_params->field_size) {
    case 224:
        group.reset(EC_GROUP_new_by_curve_name(NID_secp224r1));
        break;
    case 256:
        group.reset(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
        break;
    case 384:
        group.reset(EC_GROUP_new_by_curve_name(NID_secp384r1));
        break;
    case 521:
        group.reset(EC_GROUP_new_by_curve_name(NID_secp521r1));
        break;
    default:
        break;
    }

    if (group.get() == NULL) {
        logOpenSSLError("generate_ec_keypair");
        return -1;
    }

#if !defined(OPENSSL_IS_BORINGSSL)
    EC_GROUP_set_point_conversion_form(group.get(),
            POINT_CONVERSION_UNCOMPRESSED);
    EC_GROUP_set_asn1_flag(group.get(), OPENSSL_EC_NAMED_CURVE);
#endif

    /* initialize EC key */
    Unique_EC_KEY eckey(EC_KEY_new());
    if (eckey.get() == NULL) {
        logOpenSSLError("generate_ec_keypair");
        return -1;
    }

    if (EC_KEY_set_group(eckey.get(), group.get()) != 1) {
        logOpenSSLError("generate_ec_keypair");
        return -1;
    }

    if (EC_KEY_generate_key(eckey.get()) != 1
            || EC_KEY_check_key(eckey.get()) < 0) {
        logOpenSSLError("generate_ec_keypair");
        return -1;
    }

    if (EVP_PKEY_assign_EC_KEY(pkey, eckey.get()) == 0) {
        logOpenSSLError("generate_ec_keypair");
        return -1;
    }
    release_because_ownership_transferred(eckey);

    return 0;
}

static int generate_rsa_keypair(EVP_PKEY* pkey,
        const keymaster_rsa_keygen_params_t* rsa_params) {
    Unique_BIGNUM bn(BN_new());
    if (bn.get() == NULL) {
        logOpenSSLError("generate_rsa_keypair");
        return -1;
    }

    if (BN_set_word(bn.get(), rsa_params->public_exponent) == 0) {
        logOpenSSLError("generate_rsa_keypair");
        return -1;
    }

    /* initialize RSA */
    Unique_RSA rsa(RSA_new());
    if (rsa.get() == NULL) {
        logOpenSSLError("generate_rsa_keypair");
        return -1;
    }

    if (!RSA_generate_key_ex(rsa.get(), rsa_params->modulus_size, bn.get(), NULL) ||
        RSA_check_key(rsa.get()) < 0) {
        logOpenSSLError("generate_rsa_keypair");
        return -1;
    }

    if (EVP_PKEY_assign_RSA(pkey, rsa.get()) == 0) {
        logOpenSSLError("generate_rsa_keypair");
        return -1;
    }
    release_because_ownership_transferred(rsa);

    return 0;
}

static int openssl_generate_keypair(
    const keymaster_keypair_t key_type, const void* key_params,
    std::vector<uint8_t>* keyBlob) {
    Unique_EVP_PKEY pkey(EVP_PKEY_new());
    if (pkey.get() == NULL) {
        logOpenSSLError("openssl_generate_keypair");
        return -1;
    }

    if (key_params == NULL) {
        D("key_params == null");
        return -1;
    } else if (key_type == TYPE_DSA) {
        const keymaster_dsa_keygen_params_t* dsa_params =
            (const keymaster_dsa_keygen_params_t*)key_params;
        generate_dsa_keypair(pkey.get(), dsa_params);
    } else if (key_type == TYPE_EC) {
        const keymaster_ec_keygen_params_t* ec_params =
            (const keymaster_ec_keygen_params_t*)key_params;
        generate_ec_keypair(pkey.get(), ec_params);
    } else if (key_type == TYPE_RSA) {
        const keymaster_rsa_keygen_params_t* rsa_params =
            (const keymaster_rsa_keygen_params_t*)key_params;
        generate_rsa_keypair(pkey.get(), rsa_params);
    } else {
        D("Unsupported key type %d", key_type);
        return -1;
    }

    if (wrap_key(pkey.get(), EVP_PKEY_type(pkey->type), keyBlob)) {
        return -1;
    }

    return 0;
}

static int openssl_import_keypair(const uint8_t* key,
                                 const size_t key_length,
                                 std::vector<uint8_t>* key_blob) {
    if (key == NULL) {
        D("input key == NULL");
        return -1;
    }

    Unique_PKCS8_PRIV_KEY_INFO pkcs8(
            d2i_PKCS8_PRIV_KEY_INFO(NULL, &key, key_length));
    if (pkcs8.get() == NULL) {
        logOpenSSLError("openssl_import_keypair");
        return -1;
    }

    /* assign to EVP */
    Unique_EVP_PKEY pkey(EVP_PKCS82PKEY(pkcs8.get()));
    if (pkey.get() == NULL) {
        logOpenSSLError("openssl_import_keypair");
        return -1;
    }

    if (wrap_key(pkey.get(), EVP_PKEY_type(pkey->type), key_blob)) {
        return -1;
    }

    return 0;
}

static int openssl_get_keypair_public(const uint8_t* key_blob,
                                     const size_t key_blob_length,
                                     std::vector<uint8_t>* x509_data) {
    Unique_EVP_PKEY pkey(unwrap_key(key_blob, key_blob_length));
    if (pkey.get() == NULL) {
        return -1;
    }

    int len = i2d_PUBKEY(pkey.get(), NULL);
    if (len <= 0) {
        logOpenSSLError("openssl_get_keypair_public");
        return -1;
    }

    x509_data->resize(len);
    if (x509_data->empty()) {
        D("Could not allocate memory for public key data");
        return -1;
    }

    unsigned char* tmp = reinterpret_cast<unsigned char*>(x509_data->data());
    if (i2d_PUBKEY(pkey.get(), &tmp) != len) {
        logOpenSSLError("openssl_get_keypair_public");
        return -1;
    }

    D("Length of x509 data is %d", len);

    return 0;
}

static int sign_dsa(EVP_PKEY* pkey, keymaster_dsa_sign_params_t* sign_params,
        const uint8_t* data, const size_t dataLength,
        std::vector<uint8_t>* signedData) {
    if (sign_params->digest_type != DIGEST_NONE) {
        D("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    }

    Unique_DSA dsa(EVP_PKEY_get1_DSA(pkey));
    if (dsa.get() == NULL) {
        logOpenSSLError("openssl_sign_dsa");
        return -1;
    }

    unsigned int dsaSize = DSA_size(dsa.get());
    signedData->resize(dsaSize);

    unsigned char* tmp = reinterpret_cast<unsigned char*>(signedData->data());

    if (signedData->empty()
            || DSA_sign(0, data, dataLength, tmp, &dsaSize, dsa.get()) <= 0) {
        logOpenSSLError("openssl_sign_dsa");
        return -1;
    }

    return 0;
}

static int sign_ec(EVP_PKEY* pkey, keymaster_ec_sign_params_t* sign_params,
        const uint8_t* data, const size_t dataLength,
        std::vector<uint8_t>* signedData) {
    if (sign_params->digest_type != DIGEST_NONE) {
        D("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    }

    Unique_EC_KEY eckey(EVP_PKEY_get1_EC_KEY(pkey));
    if (eckey.get() == NULL) {
        logOpenSSLError("openssl_sign_ec");
        return -1;
    }

    unsigned int ecdsaSize = ECDSA_size(eckey.get());
    signedData->resize(ecdsaSize);

    unsigned char* tmp = reinterpret_cast<unsigned char*>(signedData->data());

    if (signedData->empty() ||
            ECDSA_sign(0, data, dataLength, tmp, &ecdsaSize, eckey.get()) <= 0) {
        logOpenSSLError("openssl_sign_ec");
        return -1;
    }
    signedData->resize(ecdsaSize);

    return 0;
}

static int sign_rsa(EVP_PKEY* pkey, keymaster_rsa_sign_params_t* sign_params,
        const uint8_t* data, const size_t dataLength,
        std::vector<uint8_t>* signedData) {
    if (sign_params->digest_type != DIGEST_NONE) {
        D("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    } else if (sign_params->padding_type != PADDING_NONE) {
        D("Cannot handle padding type %d", sign_params->padding_type);
        return -1;
    }

    Unique_RSA rsa(EVP_PKEY_get1_RSA(pkey));
    if (rsa.get() == NULL) {
        logOpenSSLError("openssl_sign_rsa");
        return -1;
    }

    signedData->resize(dataLength);

    unsigned char* tmp = reinterpret_cast<unsigned char*>(signedData->data());

    if (signedData->empty()
            || RSA_private_encrypt(dataLength, data, tmp, rsa.get(),
            RSA_NO_PADDING) <= 0) {
        logOpenSSLError("openssl_sign_rsa");
        return -1;
    }

    return 0;
}

static int openssl_sign_data(const void* params, const uint8_t* keyBlob,
        const size_t keyBlobLength, const uint8_t* data, const size_t dataLength,
        std::vector<uint8_t>* signedData) {
    Unique_EVP_PKEY pkey(unwrap_key(keyBlob, keyBlobLength));
    if (pkey.get() == NULL) {
        return -1;
    }

    int type = EVP_PKEY_type(pkey->type);
    if (type == EVP_PKEY_DSA) {
        const keymaster_dsa_sign_params_t* sign_params =
                reinterpret_cast<const keymaster_dsa_sign_params_t*>(params);
        return sign_dsa(pkey.get(),
                const_cast<keymaster_dsa_sign_params_t*>(sign_params), data,
                dataLength, signedData);
    } else if (type == EVP_PKEY_EC) {
        const keymaster_ec_sign_params_t* sign_params =
                reinterpret_cast<const keymaster_ec_sign_params_t*>(params);
        return sign_ec(pkey.get(),
                const_cast<keymaster_ec_sign_params_t*>(sign_params), data,
                dataLength, signedData);
    } else if (type == EVP_PKEY_RSA) {
        const keymaster_rsa_sign_params_t* sign_params =
                reinterpret_cast<const keymaster_rsa_sign_params_t*>(params);
        return sign_rsa(pkey.get(),
                const_cast<keymaster_rsa_sign_params_t*>(sign_params), data,
                dataLength, signedData);
    } else {
        D("Unsupported key type");
        return -1;
    }
}

static int verify_dsa(EVP_PKEY* pkey, keymaster_dsa_sign_params_t* sign_params,
                      const uint8_t* signedData, const size_t signedDataLength,
                      const uint8_t* signature, const size_t signatureLength) {
    if (sign_params->digest_type != DIGEST_NONE) {
        D("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    }

    Unique_DSA dsa(EVP_PKEY_get1_DSA(pkey));
    if (dsa.get() == NULL) {
        logOpenSSLError("openssl_verify_dsa");
        return -1;
    }

    if (DSA_verify(0, signedData, signedDataLength, signature, signatureLength, dsa.get()) <= 0) {
        logOpenSSLError("openssl_verify_dsa");
        return -1;
    }

    return 0;
}

static int verify_ec(EVP_PKEY* pkey, keymaster_ec_sign_params_t* sign_params,
                     const uint8_t* signedData, const size_t signedDataLength,
                     const uint8_t* signature, const size_t signatureLength) {
    if (sign_params->digest_type != DIGEST_NONE) {
        D("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    }

    Unique_EC_KEY eckey(EVP_PKEY_get1_EC_KEY(pkey));
    if (eckey.get() == NULL) {
        logOpenSSLError("openssl_verify_ec");
        return -1;
    }

    if (ECDSA_verify(0, signedData, signedDataLength, signature, signatureLength, eckey.get()) <=
        0) {
        logOpenSSLError("openssl_verify_ec");
        return -1;
    }

    return 0;
}

static int verify_rsa(EVP_PKEY* pkey, keymaster_rsa_sign_params_t* sign_params,
                      const uint8_t* signedData, const size_t signedDataLength,
                      const uint8_t* signature, const size_t signatureLength) {
    if (sign_params->digest_type != DIGEST_NONE) {
        D("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    } else if (sign_params->padding_type != PADDING_NONE) {
        D("Cannot handle padding type %d", sign_params->padding_type);
        return -1;
    } else if (signatureLength != signedDataLength) {
        D("signed data length must be signature length");
        return -1;
    }

    Unique_RSA rsa(EVP_PKEY_get1_RSA(pkey));
    if (rsa.get() == NULL) {
        logOpenSSLError("openssl_verify_data");
        return -1;
    }

    std::unique_ptr<uint8_t[]> dataPtr(new uint8_t[signedDataLength]);
    if (dataPtr.get() == NULL) {
        logOpenSSLError("openssl_verify_data");
        return -1;
    }

    unsigned char* tmp = reinterpret_cast<unsigned char*>(dataPtr.get());
    if (!RSA_public_decrypt(signatureLength, signature, tmp, rsa.get(),
            RSA_NO_PADDING)) {
        logOpenSSLError("openssl_verify_data");
        return -1;
    }

    int result = 0;
    for (size_t i = 0; i < signedDataLength; i++) {
        result |= tmp[i] ^ signedData[i];
    }

    return result == 0 ? 0 : -1;
}

static int openssl_verify_data(const void* params, const uint8_t* keyBlob,
        const size_t keyBlobLength, const uint8_t* signedData,
        const size_t signedDataLength, const uint8_t* signature,
        const size_t signatureLength) {
    if (signedData == NULL || signature == NULL) {
        D("data or signature buffers == NULL");
        return -1;
    }

    Unique_EVP_PKEY pkey(unwrap_key(keyBlob, keyBlobLength));
    if (pkey.get() == NULL) {
        return -1;
    }

    int type = EVP_PKEY_type(pkey->type);
    if (type == EVP_PKEY_DSA) {
        const keymaster_dsa_sign_params_t* sign_params =
            reinterpret_cast<const keymaster_dsa_sign_params_t*>(params);
        return verify_dsa(pkey.get(),
                const_cast<keymaster_dsa_sign_params_t*>(sign_params),
                signedData, signedDataLength, signature, signatureLength);
    } else if (type == EVP_PKEY_RSA) {
        const keymaster_rsa_sign_params_t* sign_params =
                reinterpret_cast<const keymaster_rsa_sign_params_t*>(params);
        return verify_rsa(pkey.get(),
                const_cast<keymaster_rsa_sign_params_t*>(sign_params),
                signedData, signedDataLength, signature, signatureLength);
    } else if (type == EVP_PKEY_EC) {
        const keymaster_ec_sign_params_t* sign_params =
                reinterpret_cast<const keymaster_ec_sign_params_t*>(params);
        return verify_ec(pkey.get(),
                const_cast<keymaster_ec_sign_params_t*>(sign_params),
                signedData, signedDataLength, signature, signatureLength);
    } else {
        D("Unsupported key type %d", type);
        return -1;
    }
}

namespace android {
namespace Keymaster {

int generateKeypair(const keymaster_keypair_t keyType,
        const void* keyParams, std::vector<uint8_t>* keyBlob) {
    return openssl_generate_keypair(keyType, keyParams, keyBlob);
}

int importKeypair(const uint8_t* key, const uint32_t keyLength,
        std::vector<uint8_t>* keyBlob) {
    return openssl_import_keypair(key, keyLength, keyBlob);
}

int getKeypairPublic(const uint8_t* keyBlob,
        const uint32_t keyBlobLength, std::vector<uint8_t>* x509Data) {
    return openssl_get_keypair_public(keyBlob, keyBlobLength,
                                         x509Data);
}

int signData(const void* params,
        const uint8_t* keyBlob, const uint32_t keyBlobLength,
        const uint8_t* data, const uint32_t dataLength,
        std::vector<uint8_t>* signedData) {
    return openssl_sign_data(params, keyBlob, keyBlobLength, data, dataLength,
            signedData);
}

int verifyData(const void* params, const uint8_t* keyBlob,
        const uint32_t keyBlobLength, const uint8_t* signedData,
        const uint32_t signedDataLength, const uint8_t* signature,
        const uint32_t signatureLength) {
    return openssl_verify_data(params, keyBlob, keyBlobLength, signedData,
            signedDataLength, signature, signatureLength);
}

}
}

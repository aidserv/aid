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

#pragma once
#include <stdint.h>

/**
 * Asymmetric key pair types.
 */
typedef enum {
    TYPE_RSA = 1,
    TYPE_DSA = 2,
    TYPE_EC = 3,
} keymaster_keypair_t;

/**
 * Parameters needed to generate an RSA key.
 */
typedef struct {
    uint32_t modulus_size;
    uint64_t public_exponent;
} keymaster_rsa_keygen_params_t;

/**
 * Parameters needed to generate a DSA key.
 */
typedef struct {
    uint32_t key_size;
    uint32_t generator_len;
    uint32_t prime_p_len;
    uint32_t prime_q_len;
    const uint8_t* generator;
    const uint8_t* prime_p;
    const uint8_t* prime_q;
} keymaster_dsa_keygen_params_t;

/**
 * Parameters needed to generate an EC key.
 *
 * Field size is the only parameter in version 2. The sizes correspond to these required curves:
 *
 * 192 = NIST P-192
 * 224 = NIST P-224
 * 256 = NIST P-256
 * 384 = NIST P-384
 * 521 = NIST P-521
 *
 * The parameters for these curves are available at: http://www.nsa.gov/ia/_files/nist-routines.pdf
 * in Chapter 4.
 */
typedef struct {
    uint32_t field_size;
} keymaster_ec_keygen_params_t;

/**
 * Digest type.
 */
typedef enum {
    DIGEST_NONE,
} keymaster_digest_algorithm_t;

/**
 * Type of padding used for RSA operations.
 */
typedef enum {
    PADDING_NONE,
} keymaster_rsa_padding_t;


typedef struct {
    keymaster_digest_algorithm_t digest_type;
} keymaster_dsa_sign_params_t;

typedef struct {
    keymaster_digest_algorithm_t digest_type;
} keymaster_ec_sign_params_t;

typedef struct {
    keymaster_digest_algorithm_t digest_type;
    keymaster_rsa_padding_t padding_type;
} keymaster_rsa_sign_params_t;

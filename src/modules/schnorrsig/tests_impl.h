/**********************************************************************
 * Copyright (c) 2018 Andrew Poelstra                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_MODULE_SCHNORRSIG_TESTS_
#define _SECP256K1_MODULE_SCHNORRSIG_TESTS_

#include "secp256k1_schnorrsig.h"

void test_schnorrsig_serialize(void) {
    secp256k1_schnorrsig sig;
    unsigned char in[64];
    unsigned char out[64];

    memset(in, 0x12, 64);
    CHECK(secp256k1_schnorrsig_parse(ctx, &sig, in));
    CHECK(secp256k1_schnorrsig_serialize(ctx, out, &sig));
    CHECK(memcmp(in, out, 64) == 0);
}

void test_schnorrsig_api(secp256k1_scratch_space *scratch) {
    unsigned char sk1[32];
    unsigned char sk2[32];
    unsigned char sk3[32];
    unsigned char msg[32];
    unsigned char data32[32];
    unsigned char s2c_data32[32];
    unsigned char rand32[32];
    unsigned char rand_commitment32[32];
    unsigned char sig64[64];
    secp256k1_pubkey pk[3];
    secp256k1_schnorrsig sig;
    const secp256k1_schnorrsig *sigptr = &sig;
    const unsigned char *msgptr = msg;
    const secp256k1_pubkey *pkptr = &pk[0];
    secp256k1_s2c_opening s2c_opening;
    secp256k1_pubkey client_commit;
    unsigned char ones[32];

    /** setup **/
    secp256k1_context *none = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    secp256k1_context *sign = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    secp256k1_context *vrfy = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    secp256k1_context *both = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    int ecount;
    memset(ones, 0xff, 32);

    secp256k1_context_set_error_callback(none, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_error_callback(sign, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_error_callback(vrfy, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_error_callback(both, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(none, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(sign, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(vrfy, counting_illegal_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(both, counting_illegal_callback_fn, &ecount);

    secp256k1_rand256(sk1);
    secp256k1_rand256(sk2);
    secp256k1_rand256(sk3);
    secp256k1_rand256(msg);
    CHECK(secp256k1_ec_pubkey_create(ctx, &pk[0], sk1) == 1);
    CHECK(secp256k1_ec_pubkey_create(ctx, &pk[1], sk2) == 1);
    CHECK(secp256k1_ec_pubkey_create(ctx, &pk[2], sk3) == 1);

    /** main test body **/
    ecount = 0;
    CHECK(secp256k1_schnorrsig_sign(none, &sig, &s2c_opening, msg, sk1, s2c_data32, NULL, NULL) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_schnorrsig_sign(vrfy, &sig, &s2c_opening, msg, sk1, s2c_data32, NULL, NULL) == 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_sign(sign, &sig, &s2c_opening, msg, sk1, s2c_data32, NULL, NULL) == 1);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_sign(sign, NULL, &s2c_opening, msg, sk1, s2c_data32, NULL, NULL) == 0);
    CHECK(ecount == 3);
    CHECK(secp256k1_schnorrsig_sign(sign, &sig, NULL, msg, sk1, s2c_data32, NULL, NULL) == 1);
    CHECK(ecount == 3);
    CHECK(secp256k1_schnorrsig_sign(sign, &sig, &s2c_opening, NULL, sk1, s2c_data32, NULL, NULL) == 0);
    CHECK(ecount == 4);
    CHECK(secp256k1_schnorrsig_sign(sign, &sig, &s2c_opening, msg, NULL, s2c_data32, NULL, NULL) == 0);
    CHECK(ecount == 5);
    CHECK(secp256k1_schnorrsig_sign(sign, &sig, &s2c_opening, msg, sk1, NULL, NULL, NULL) == 1);
    CHECK(ecount == 5);
    /* s2c commitments with a different nonce function than bipschnorr are not allowed */
    CHECK(secp256k1_schnorrsig_sign(sign, &sig, &s2c_opening, msg, sk1, s2c_data32, secp256k1_nonce_function_rfc6979, NULL) == 0);
    CHECK(ecount == 6);

    ecount = 0;
    CHECK(secp256k1_schnorrsig_serialize(none, sig64, &sig) == 1);
    CHECK(ecount == 0);
    CHECK(secp256k1_schnorrsig_serialize(none, NULL, &sig) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_schnorrsig_serialize(none, sig64, NULL) == 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_parse(none, &sig, sig64) == 1);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_parse(none, NULL, sig64) == 0);
    CHECK(ecount == 3);
    CHECK(secp256k1_schnorrsig_parse(none, &sig, NULL) == 0);
    CHECK(ecount == 4);

    /* Create sign-to-contract commitment to data32 for testing verify_s2c_commit */
    secp256k1_rand256(data32);
    CHECK(secp256k1_schnorrsig_sign(sign, &sig, &s2c_opening, msg, sk1, data32, NULL, NULL) == 1);
    ecount = 0;
    CHECK(secp256k1_schnorrsig_verify_s2c_commit(none, &sig, data32, &s2c_opening) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_schnorrsig_verify_s2c_commit(vrfy, &sig, data32, &s2c_opening) == 1);
    CHECK(ecount == 1);
    {
        /* Overflowing x-coordinate in signature */
        secp256k1_schnorrsig sig_tmp = sig;
        memcpy(&sig_tmp.data[0], ones, 32);
        CHECK(secp256k1_schnorrsig_verify_s2c_commit(vrfy, &sig_tmp, data32, &s2c_opening) == 0);
    }
    CHECK(secp256k1_schnorrsig_verify_s2c_commit(vrfy, NULL, data32, &s2c_opening) == 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_verify_s2c_commit(vrfy, &sig, NULL, &s2c_opening) == 0);
    CHECK(ecount == 3);
    CHECK(secp256k1_schnorrsig_verify_s2c_commit(vrfy, &sig, data32, NULL) == 0);
    CHECK(ecount == 4);
    {
        /* Verification with uninitialized s2c_opening should fail */
        secp256k1_s2c_opening s2c_opening_tmp;
        CHECK(secp256k1_schnorrsig_verify_s2c_commit(vrfy, &sig, data32, &s2c_opening_tmp) == 0);
        CHECK(ecount == 5);
    }

    secp256k1_rand256(rand32);
    ecount = 0;
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_commit(none, rand_commitment32, rand32) == 1);
    CHECK(ecount == 0);
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_commit(none, NULL, rand32) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_commit(none, rand_commitment32, NULL) == 0);
    CHECK(ecount == 2);

    ecount = 0;
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_client_commit(sign, &client_commit, msg, sk1, rand_commitment32) == 1);
    CHECK(ecount == 0);
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_client_commit(none, &client_commit, msg, sk1, rand_commitment32) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_client_commit(sign, NULL, msg, sk1, rand_commitment32) == 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_client_commit(sign, &client_commit, NULL, sk1, rand_commitment32) == 0);
    CHECK(ecount == 3);
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_client_commit(sign, &client_commit, msg, NULL, rand_commitment32) == 0);
    CHECK(ecount == 4);
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_client_commit(sign, &client_commit, msg, sk1, NULL) == 0);
    CHECK(ecount == 5);

    CHECK(secp256k1_schnorrsig_sign(sign, &sig, &s2c_opening, msg, sk1, rand32, NULL, NULL) == 1);

    ecount = 0;
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_verify(ctx, &sig, rand32, &s2c_opening, &client_commit) == 1);
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_verify(none, &sig, rand32, &s2c_opening, &client_commit) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_verify(sign, &sig, rand32, &s2c_opening, &client_commit) == 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_verify(vrfy, &sig, rand32, &s2c_opening, &client_commit) == 1);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_verify(vrfy, NULL, rand32, &s2c_opening, &client_commit) == 0);
    CHECK(ecount == 3);
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_verify(vrfy, &sig, NULL, &s2c_opening, &client_commit) == 0);
    CHECK(ecount == 4);
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_verify(vrfy, &sig, rand32, NULL, &client_commit) == 0);
    CHECK(ecount == 5);
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_verify(vrfy, &sig, rand32, &s2c_opening, NULL) == 0);
    CHECK(ecount == 6);

    ecount = 0;
    CHECK(secp256k1_schnorrsig_verify(none, &sig, msg, &pk[0]) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_schnorrsig_verify(sign, &sig, msg, &pk[0]) == 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_verify(vrfy, &sig, msg, &pk[0]) == 1);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_verify(vrfy, NULL, msg, &pk[0]) == 0);
    CHECK(ecount == 3);
    CHECK(secp256k1_schnorrsig_verify(vrfy, &sig, NULL, &pk[0]) == 0);
    CHECK(ecount == 4);
    CHECK(secp256k1_schnorrsig_verify(vrfy, &sig, msg, NULL) == 0);
    CHECK(ecount == 5);

    ecount = 0;
    CHECK(secp256k1_schnorrsig_verify_batch(none, scratch, &sigptr, &msgptr, &pkptr, 1) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_schnorrsig_verify_batch(sign, scratch, &sigptr, &msgptr, &pkptr, 1) == 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_verify_batch(vrfy, scratch, &sigptr, &msgptr, &pkptr, 1) == 1);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_verify_batch(vrfy, scratch, NULL, NULL, NULL, 0) == 1);
    CHECK(ecount == 2);
    CHECK(secp256k1_schnorrsig_verify_batch(vrfy, scratch, NULL, &msgptr, &pkptr, 1) == 0);
    CHECK(ecount == 3);
    CHECK(secp256k1_schnorrsig_verify_batch(vrfy, scratch, &sigptr, NULL, &pkptr, 1) == 0);
    CHECK(ecount == 4);
    CHECK(secp256k1_schnorrsig_verify_batch(vrfy, scratch, &sigptr, &msgptr, NULL, 1) == 0);
    CHECK(ecount == 5);
    CHECK(secp256k1_schnorrsig_verify_batch(vrfy, scratch, &sigptr, &msgptr, &pkptr, (size_t)1 << (sizeof(size_t)*8-1)) == 0);
    CHECK(ecount == 6);
    CHECK(secp256k1_schnorrsig_verify_batch(vrfy, scratch, &sigptr, &msgptr, &pkptr, 1 << 31) == 0);
    CHECK(ecount == 7);

    secp256k1_context_destroy(none);
    secp256k1_context_destroy(sign);
    secp256k1_context_destroy(vrfy);
    secp256k1_context_destroy(both);
}

/* Helper function for schnorrsig_bip_vectors
 * Signs the message and checks that it's the same as expected_sig. */
void test_schnorrsig_bip_vectors_check_signing(const unsigned char *sk, const unsigned char *pk_serialized, const unsigned char *msg, const unsigned char *expected_sig, const int expected_nonce_is_negated) {
    secp256k1_schnorrsig sig;
    unsigned char serialized_sig[64];
    secp256k1_pubkey pk;
    secp256k1_s2c_opening s2c_opening;

    CHECK(secp256k1_schnorrsig_sign(ctx, &sig, &s2c_opening, msg, sk, NULL, NULL, NULL));
    CHECK(s2c_opening.nonce_is_negated == expected_nonce_is_negated);
    CHECK(secp256k1_schnorrsig_serialize(ctx, serialized_sig, &sig));
    CHECK(memcmp(serialized_sig, expected_sig, 64) == 0);

    CHECK(secp256k1_ec_pubkey_parse(ctx, &pk, pk_serialized, 33));
    CHECK(secp256k1_schnorrsig_verify(ctx, &sig, msg, &pk));
}

/* Helper function for schnorrsig_bip_vectors
 * Checks that both verify and verify_batch return the same value as expected. */
void test_schnorrsig_bip_vectors_check_verify(secp256k1_scratch_space *scratch, const unsigned char *pk_serialized, const unsigned char *msg32, const unsigned char *sig_serialized, int expected) {
    const unsigned char *msg_arr[1];
    const secp256k1_schnorrsig *sig_arr[1];
    const secp256k1_pubkey *pk_arr[1];
    secp256k1_pubkey pk;
    secp256k1_schnorrsig sig;

    CHECK(secp256k1_ec_pubkey_parse(ctx, &pk, pk_serialized, 33));
    CHECK(secp256k1_schnorrsig_parse(ctx, &sig, sig_serialized));

    sig_arr[0] = &sig;
    msg_arr[0] = msg32;
    pk_arr[0] = &pk;

    CHECK(expected == secp256k1_schnorrsig_verify(ctx, &sig, msg32, &pk));
    CHECK(expected == secp256k1_schnorrsig_verify_batch(ctx, scratch, sig_arr, msg_arr, pk_arr, 1));
}

/* Test vectors according to BIP-schnorr
 * (https://github.com/sipa/bips/blob/7f6a73e53c8bbcf2d008ea0546f76433e22094a8/bip-schnorr/test-vectors.csv).
 */
void test_schnorrsig_bip_vectors(secp256k1_scratch_space *scratch) {
    {
        /* Test vector 1 */
        const unsigned char sk1[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
        };
        const unsigned char pk1[33] = {
            0x02, 0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB,
            0xAC, 0x55, 0xA0, 0x62, 0x95, 0xCE, 0x87, 0x0B,
            0x07, 0x02, 0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28,
            0xD9, 0x59, 0xF2, 0x81, 0x5B, 0x16, 0xF8, 0x17,
            0x98
        };
        const unsigned char msg1[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        const unsigned char sig1[64] = {
            0x78, 0x7A, 0x84, 0x8E, 0x71, 0x04, 0x3D, 0x28,
            0x0C, 0x50, 0x47, 0x0E, 0x8E, 0x15, 0x32, 0xB2,
            0xDD, 0x5D, 0x20, 0xEE, 0x91, 0x2A, 0x45, 0xDB,
            0xDD, 0x2B, 0xD1, 0xDF, 0xBF, 0x18, 0x7E, 0xF6,
            0x70, 0x31, 0xA9, 0x88, 0x31, 0x85, 0x9D, 0xC3,
            0x4D, 0xFF, 0xEE, 0xDD, 0xA8, 0x68, 0x31, 0x84,
            0x2C, 0xCD, 0x00, 0x79, 0xE1, 0xF9, 0x2A, 0xF1,
            0x77, 0xF7, 0xF2, 0x2C, 0xC1, 0xDC, 0xED, 0x05
        };
        test_schnorrsig_bip_vectors_check_signing(sk1, pk1, msg1, sig1, 1);
        test_schnorrsig_bip_vectors_check_verify(scratch, pk1, msg1, sig1, 1);
    }
    {
        /* Test vector 2 */
        const unsigned char sk2[32] = {
            0xB7, 0xE1, 0x51, 0x62, 0x8A, 0xED, 0x2A, 0x6A,
            0xBF, 0x71, 0x58, 0x80, 0x9C, 0xF4, 0xF3, 0xC7,
            0x62, 0xE7, 0x16, 0x0F, 0x38, 0xB4, 0xDA, 0x56,
            0xA7, 0x84, 0xD9, 0x04, 0x51, 0x90, 0xCF, 0xEF
        };
        const unsigned char pk2[33] = {
            0x02, 0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C,
            0x5F, 0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41,
            0xBE, 0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE,
            0xD8, 0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6,
            0x59
        };
        const unsigned char msg2[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig2[64] = {
            0x2A, 0x29, 0x8D, 0xAC, 0xAE, 0x57, 0x39, 0x5A,
            0x15, 0xD0, 0x79, 0x5D, 0xDB, 0xFD, 0x1D, 0xCB,
            0x56, 0x4D, 0xA8, 0x2B, 0x0F, 0x26, 0x9B, 0xC7,
            0x0A, 0x74, 0xF8, 0x22, 0x04, 0x29, 0xBA, 0x1D,
            0x1E, 0x51, 0xA2, 0x2C, 0xCE, 0xC3, 0x55, 0x99,
            0xB8, 0xF2, 0x66, 0x91, 0x22, 0x81, 0xF8, 0x36,
            0x5F, 0xFC, 0x2D, 0x03, 0x5A, 0x23, 0x04, 0x34,
            0xA1, 0xA6, 0x4D, 0xC5, 0x9F, 0x70, 0x13, 0xFD
        };
        test_schnorrsig_bip_vectors_check_signing(sk2, pk2, msg2, sig2, 0);
        test_schnorrsig_bip_vectors_check_verify(scratch, pk2, msg2, sig2, 1);
    }
    {
        /* Test vector 3 */
        const unsigned char sk3[32] = {
            0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
            0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
            0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
            0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x14, 0xE5, 0xC7
        };
        const unsigned char pk3[33] = {
            0x03, 0xFA, 0xC2, 0x11, 0x4C, 0x2F, 0xBB, 0x09,
            0x15, 0x27, 0xEB, 0x7C, 0x64, 0xEC, 0xB1, 0x1F,
            0x80, 0x21, 0xCB, 0x45, 0xE8, 0xE7, 0x80, 0x9D,
            0x3C, 0x09, 0x38, 0xE4, 0xB8, 0xC0, 0xE5, 0xF8,
            0x4B
        };
        const unsigned char msg3[32] = {
            0x5E, 0x2D, 0x58, 0xD8, 0xB3, 0xBC, 0xDF, 0x1A,
            0xBA, 0xDE, 0xC7, 0x82, 0x90, 0x54, 0xF9, 0x0D,
            0xDA, 0x98, 0x05, 0xAA, 0xB5, 0x6C, 0x77, 0x33,
            0x30, 0x24, 0xB9, 0xD0, 0xA5, 0x08, 0xB7, 0x5C
        };
        const unsigned char sig3[64] = {
            0x00, 0xDA, 0x9B, 0x08, 0x17, 0x2A, 0x9B, 0x6F,
            0x04, 0x66, 0xA2, 0xDE, 0xFD, 0x81, 0x7F, 0x2D,
            0x7A, 0xB4, 0x37, 0xE0, 0xD2, 0x53, 0xCB, 0x53,
            0x95, 0xA9, 0x63, 0x86, 0x6B, 0x35, 0x74, 0xBE,
            0x00, 0x88, 0x03, 0x71, 0xD0, 0x17, 0x66, 0x93,
            0x5B, 0x92, 0xD2, 0xAB, 0x4C, 0xD5, 0xC8, 0xA2,
            0xA5, 0x83, 0x7E, 0xC5, 0x7F, 0xED, 0x76, 0x60,
            0x77, 0x3A, 0x05, 0xF0, 0xDE, 0x14, 0x23, 0x80
        };
        test_schnorrsig_bip_vectors_check_signing(sk3, pk3, msg3, sig3, 0);
        test_schnorrsig_bip_vectors_check_verify(scratch, pk3, msg3, sig3, 1);
    }
    {
        /* Test vector 4 */
        const unsigned char pk4[33] = {
            0x03, 0xDE, 0xFD, 0xEA, 0x4C, 0xDB, 0x67, 0x77,
            0x50, 0xA4, 0x20, 0xFE, 0xE8, 0x07, 0xEA, 0xCF,
            0x21, 0xEB, 0x98, 0x98, 0xAE, 0x79, 0xB9, 0x76,
            0x87, 0x66, 0xE4, 0xFA, 0xA0, 0x4A, 0x2D, 0x4A,
            0x34
        };
        const unsigned char msg4[32] = {
            0x4D, 0xF3, 0xC3, 0xF6, 0x8F, 0xCC, 0x83, 0xB2,
            0x7E, 0x9D, 0x42, 0xC9, 0x04, 0x31, 0xA7, 0x24,
            0x99, 0xF1, 0x78, 0x75, 0xC8, 0x1A, 0x59, 0x9B,
            0x56, 0x6C, 0x98, 0x89, 0xB9, 0x69, 0x67, 0x03
        };
        const unsigned char sig4[64] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x3B, 0x78, 0xCE, 0x56, 0x3F,
            0x89, 0xA0, 0xED, 0x94, 0x14, 0xF5, 0xAA, 0x28,
            0xAD, 0x0D, 0x96, 0xD6, 0x79, 0x5F, 0x9C, 0x63,
            0x02, 0xA8, 0xDC, 0x32, 0xE6, 0x4E, 0x86, 0xA3,
            0x33, 0xF2, 0x0E, 0xF5, 0x6E, 0xAC, 0x9B, 0xA3,
            0x0B, 0x72, 0x46, 0xD6, 0xD2, 0x5E, 0x22, 0xAD,
            0xB8, 0xC6, 0xBE, 0x1A, 0xEB, 0x08, 0xD4, 0x9D
        };
        test_schnorrsig_bip_vectors_check_verify(scratch, pk4, msg4, sig4, 1);
    }
    {
        /* Test vector 5 */
        const unsigned char pk5[33] = {
            0x03, 0x1B, 0x84, 0xC5, 0x56, 0x7B, 0x12, 0x64,
            0x40, 0x99, 0x5D, 0x3E, 0xD5, 0xAA, 0xBA, 0x05,
            0x65, 0xD7, 0x1E, 0x18, 0x34, 0x60, 0x48, 0x19,
            0xFF, 0x9C, 0x17, 0xF5, 0xE9, 0xD5, 0xDD, 0x07,
            0x8F
        };
        const unsigned char msg5[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        const unsigned char sig5[64] = {
            0x52, 0x81, 0x85, 0x79, 0xAC, 0xA5, 0x97, 0x67,
            0xE3, 0x29, 0x1D, 0x91, 0xB7, 0x6B, 0x63, 0x7B,
            0xEF, 0x06, 0x20, 0x83, 0x28, 0x49, 0x92, 0xF2,
            0xD9, 0x5F, 0x56, 0x4C, 0xA6, 0xCB, 0x4E, 0x35,
            0x30, 0xB1, 0xDA, 0x84, 0x9C, 0x8E, 0x83, 0x04,
            0xAD, 0xC0, 0xCF, 0xE8, 0x70, 0x66, 0x03, 0x34,
            0xB3, 0xCF, 0xC1, 0x8E, 0x82, 0x5E, 0xF1, 0xDB,
            0x34, 0xCF, 0xAE, 0x3D, 0xFC, 0x5D, 0x81, 0x87
        };
        test_schnorrsig_bip_vectors_check_verify(scratch, pk5, msg5, sig5, 1);
    }
    {
        /* Test vector 6 */
        const unsigned char pk6[33] = {
            0x03, 0xFA, 0xC2, 0x11, 0x4C, 0x2F, 0xBB, 0x09,
            0x15, 0x27, 0xEB, 0x7C, 0x64, 0xEC, 0xB1, 0x1F,
            0x80, 0x21, 0xCB, 0x45, 0xE8, 0xE7, 0x80, 0x9D,
            0x3C, 0x09, 0x38, 0xE4, 0xB8, 0xC0, 0xE5, 0xF8,
            0x4B
        };
        const unsigned char msg6[32] = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        };
        const unsigned char sig6[64] = {
            0x57, 0x0D, 0xD4, 0xCA, 0x83, 0xD4, 0xE6, 0x31,
            0x7B, 0x8E, 0xE6, 0xBA, 0xE8, 0x34, 0x67, 0xA1,
            0xBF, 0x41, 0x9D, 0x07, 0x67, 0x12, 0x2D, 0xE4,
            0x09, 0x39, 0x44, 0x14, 0xB0, 0x50, 0x80, 0xDC,
            0xE9, 0xEE, 0x5F, 0x23, 0x7C, 0xBD, 0x10, 0x8E,
            0xAB, 0xAE, 0x1E, 0x37, 0x75, 0x9A, 0xE4, 0x7F,
            0x8E, 0x42, 0x03, 0xDA, 0x35, 0x32, 0xEB, 0x28,
            0xDB, 0x86, 0x0F, 0x33, 0xD6, 0x2D, 0x49, 0xBD
        };
        test_schnorrsig_bip_vectors_check_verify(scratch, pk6, msg6, sig6, 1);
    }
    {
        /* Test vector 7 */
        const unsigned char pk7[33] = {
            0x03, 0xEE, 0xFD, 0xEA, 0x4C, 0xDB, 0x67, 0x77,
            0x50, 0xA4, 0x20, 0xFE, 0xE8, 0x07, 0xEA, 0xCF,
            0x21, 0xEB, 0x98, 0x98, 0xAE, 0x79, 0xB9, 0x76,
            0x87, 0x66, 0xE4, 0xFA, 0xA0, 0x4A, 0x2D, 0x4A,
            0x34
        };
        secp256k1_pubkey pk7_parsed;
        /* No need to check the signature of the test vector as parsing the pubkey already fails */
        CHECK(!secp256k1_ec_pubkey_parse(ctx, &pk7_parsed, pk7, 33));
    }
    {
        /* Test vector 8 */
        const unsigned char pk8[33] = {
            0x02, 0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C,
            0x5F, 0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41,
            0xBE, 0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE,
            0xD8, 0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6,
            0x59
        };
        const unsigned char msg8[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig8[64] = {
            0x2A, 0x29, 0x8D, 0xAC, 0xAE, 0x57, 0x39, 0x5A,
            0x15, 0xD0, 0x79, 0x5D, 0xDB, 0xFD, 0x1D, 0xCB,
            0x56, 0x4D, 0xA8, 0x2B, 0x0F, 0x26, 0x9B, 0xC7,
            0x0A, 0x74, 0xF8, 0x22, 0x04, 0x29, 0xBA, 0x1D,
            0xFA, 0x16, 0xAE, 0xE0, 0x66, 0x09, 0x28, 0x0A,
            0x19, 0xB6, 0x7A, 0x24, 0xE1, 0x97, 0x7E, 0x46,
            0x97, 0x71, 0x2B, 0x5F, 0xD2, 0x94, 0x39, 0x14,
            0xEC, 0xD5, 0xF7, 0x30, 0x90, 0x1B, 0x4A, 0xB7
        };
        test_schnorrsig_bip_vectors_check_verify(scratch, pk8, msg8, sig8, 0);
    }
    {
        /* Test vector 9 */
        const unsigned char pk9[33] = {
            0x03, 0xFA, 0xC2, 0x11, 0x4C, 0x2F, 0xBB, 0x09,
            0x15, 0x27, 0xEB, 0x7C, 0x64, 0xEC, 0xB1, 0x1F,
            0x80, 0x21, 0xCB, 0x45, 0xE8, 0xE7, 0x80, 0x9D,
            0x3C, 0x09, 0x38, 0xE4, 0xB8, 0xC0, 0xE5, 0xF8,
            0x4B
        };
        const unsigned char msg9[32] = {
            0x5E, 0x2D, 0x58, 0xD8, 0xB3, 0xBC, 0xDF, 0x1A,
            0xBA, 0xDE, 0xC7, 0x82, 0x90, 0x54, 0xF9, 0x0D,
            0xDA, 0x98, 0x05, 0xAA, 0xB5, 0x6C, 0x77, 0x33,
            0x30, 0x24, 0xB9, 0xD0, 0xA5, 0x08, 0xB7, 0x5C
        };
        const unsigned char sig9[64] = {
            0x00, 0xDA, 0x9B, 0x08, 0x17, 0x2A, 0x9B, 0x6F,
            0x04, 0x66, 0xA2, 0xDE, 0xFD, 0x81, 0x7F, 0x2D,
            0x7A, 0xB4, 0x37, 0xE0, 0xD2, 0x53, 0xCB, 0x53,
            0x95, 0xA9, 0x63, 0x86, 0x6B, 0x35, 0x74, 0xBE,
            0xD0, 0x92, 0xF9, 0xD8, 0x60, 0xF1, 0x77, 0x6A,
            0x1F, 0x74, 0x12, 0xAD, 0x8A, 0x1E, 0xB5, 0x0D,
            0xAC, 0xCC, 0x22, 0x2B, 0xC8, 0xC0, 0xE2, 0x6B,
            0x20, 0x56, 0xDF, 0x2F, 0x27, 0x3E, 0xFD, 0xEC
        };
        test_schnorrsig_bip_vectors_check_verify(scratch, pk9, msg9, sig9, 0);
    }
    {
        /* Test vector 10 */
        const unsigned char pk10[33] = {
            0x02, 0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB,
            0xAC, 0x55, 0xA0, 0x62, 0x95, 0xCE, 0x87, 0x0B,
            0x07, 0x02, 0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28,
            0xD9, 0x59, 0xF2, 0x81, 0x5B, 0x16, 0xF8, 0x17,
            0x98
        };
        const unsigned char msg10[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        const unsigned char sig10[64] = {
            0x78, 0x7A, 0x84, 0x8E, 0x71, 0x04, 0x3D, 0x28,
            0x0C, 0x50, 0x47, 0x0E, 0x8E, 0x15, 0x32, 0xB2,
            0xDD, 0x5D, 0x20, 0xEE, 0x91, 0x2A, 0x45, 0xDB,
            0xDD, 0x2B, 0xD1, 0xDF, 0xBF, 0x18, 0x7E, 0xF6,
            0x8F, 0xCE, 0x56, 0x77, 0xCE, 0x7A, 0x62, 0x3C,
            0xB2, 0x00, 0x11, 0x22, 0x57, 0x97, 0xCE, 0x7A,
            0x8D, 0xE1, 0xDC, 0x6C, 0xCD, 0x4F, 0x75, 0x4A,
            0x47, 0xDA, 0x6C, 0x60, 0x0E, 0x59, 0x54, 0x3C
        };
        test_schnorrsig_bip_vectors_check_verify(scratch, pk10, msg10, sig10, 0);
    }
    {
        /* Test vector 11 */
        const unsigned char pk11[33] = {
            0x03, 0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C,
            0x5F, 0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41,
            0xBE, 0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE,
            0xD8, 0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6,
            0x59
        };
        const unsigned char msg11[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig11[64] = {
            0x2A, 0x29, 0x8D, 0xAC, 0xAE, 0x57, 0x39, 0x5A,
            0x15, 0xD0, 0x79, 0x5D, 0xDB, 0xFD, 0x1D, 0xCB,
            0x56, 0x4D, 0xA8, 0x2B, 0x0F, 0x26, 0x9B, 0xC7,
            0x0A, 0x74, 0xF8, 0x22, 0x04, 0x29, 0xBA, 0x1D,
            0x1E, 0x51, 0xA2, 0x2C, 0xCE, 0xC3, 0x55, 0x99,
            0xB8, 0xF2, 0x66, 0x91, 0x22, 0x81, 0xF8, 0x36,
            0x5F, 0xFC, 0x2D, 0x03, 0x5A, 0x23, 0x04, 0x34,
            0xA1, 0xA6, 0x4D, 0xC5, 0x9F, 0x70, 0x13, 0xFD
        };
        test_schnorrsig_bip_vectors_check_verify(scratch, pk11, msg11, sig11, 0);
    }
    {
        /* Test vector 12 */
        const unsigned char pk12[33] = {
            0x02, 0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C,
            0x5F, 0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41,
            0xBE, 0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE,
            0xD8, 0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6,
            0x59
        };
        const unsigned char msg12[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig12[64] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x9E, 0x9D, 0x01, 0xAF, 0x98, 0x8B, 0x5C, 0xED,
            0xCE, 0x47, 0x22, 0x1B, 0xFA, 0x9B, 0x22, 0x27,
            0x21, 0xF3, 0xFA, 0x40, 0x89, 0x15, 0x44, 0x4A,
            0x4B, 0x48, 0x90, 0x21, 0xDB, 0x55, 0x77, 0x5F
        };
        test_schnorrsig_bip_vectors_check_verify(scratch, pk12, msg12, sig12, 0);
    }
    {
        /* Test vector 13 */
        const unsigned char pk13[33] = {
            0x02, 0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C,
            0x5F, 0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41,
            0xBE, 0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE,
            0xD8, 0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6,
            0x59
        };
        const unsigned char msg13[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig13[64] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
            0xD3, 0x7D, 0xDF, 0x02, 0x54, 0x35, 0x18, 0x36,
            0xD8, 0x4B, 0x1B, 0xD6, 0xA7, 0x95, 0xFD, 0x5D,
            0x52, 0x30, 0x48, 0xF2, 0x98, 0xC4, 0x21, 0x4D,
            0x18, 0x7F, 0xE4, 0x89, 0x29, 0x47, 0xF7, 0x28
        };
        test_schnorrsig_bip_vectors_check_verify(scratch, pk13, msg13, sig13, 0);
    }
    {
        /* Test vector 14 */
        const unsigned char pk14[33] = {
            0x02, 0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C,
            0x5F, 0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41,
            0xBE, 0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE,
            0xD8, 0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6,
            0x59
        };
        const unsigned char msg14[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x14, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig14[64] = {
            0x4A, 0x29, 0x8D, 0xAC, 0xAE, 0x57, 0x39, 0x5A,
            0x15, 0xD0, 0x79, 0x5D, 0xDB, 0xFD, 0x1D, 0xCB,
            0x56, 0x4D, 0xA8, 0x2B, 0x0F, 0x26, 0x9B, 0xC7,
            0x0A, 0x74, 0xF8, 0x22, 0x04, 0x29, 0xBA, 0x1D,
            0x1E, 0x51, 0xA2, 0x2C, 0xCE, 0xC3, 0x55, 0x99,
            0xB8, 0xF2, 0x66, 0x91, 0x22, 0x81, 0xF8, 0x36,
            0x5F, 0xFC, 0x2D, 0x03, 0x5A, 0x23, 0x04, 0x34,
            0xA1, 0xA6, 0x4D, 0xC5, 0x9F, 0x70, 0x13, 0xFD
        };
        test_schnorrsig_bip_vectors_check_verify(scratch, pk14, msg14, sig14, 0);
    }
    {
        /* Test vector 15 */
        const unsigned char pk15[33] = {
            0x02, 0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C,
            0x5F, 0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41,
            0xBE, 0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE,
            0xD8, 0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6,
            0x59
        };
        const unsigned char msg15[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig15[64] = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x2F,
            0x1E, 0x51, 0xA2, 0x2C, 0xCE, 0xC3, 0x55, 0x99,
            0xB8, 0xF2, 0x66, 0x91, 0x22, 0x81, 0xF8, 0x36,
            0x5F, 0xFC, 0x2D, 0x03, 0x5A, 0x23, 0x04, 0x34,
            0xA1, 0xA6, 0x4D, 0xC5, 0x9F, 0x70, 0x13, 0xFD
        };
        test_schnorrsig_bip_vectors_check_verify(scratch, pk15, msg15, sig15, 0);
    }
    {
        /* Test vector 16 */
        const unsigned char pk16[33] = {
            0x02, 0xDF, 0xF1, 0xD7, 0x7F, 0x2A, 0x67, 0x1C,
            0x5F, 0x36, 0x18, 0x37, 0x26, 0xDB, 0x23, 0x41,
            0xBE, 0x58, 0xFE, 0xAE, 0x1D, 0xA2, 0xDE, 0xCE,
            0xD8, 0x43, 0x24, 0x0F, 0x7B, 0x50, 0x2B, 0xA6,
            0x59
        };
        const unsigned char msg16[32] = {
            0x24, 0x3F, 0x6A, 0x88, 0x85, 0xA3, 0x08, 0xD3,
            0x13, 0x19, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x44,
            0xA4, 0x09, 0x38, 0x22, 0x29, 0x9F, 0x31, 0xD0,
            0x08, 0x2E, 0xFA, 0x98, 0xEC, 0x4E, 0x6C, 0x89
        };
        const unsigned char sig16[64] = {
            0x2A, 0x29, 0x8D, 0xAC, 0xAE, 0x57, 0x39, 0x5A,
            0x15, 0xD0, 0x79, 0x5D, 0xDB, 0xFD, 0x1D, 0xCB,
            0x56, 0x4D, 0xA8, 0x2B, 0x0F, 0x26, 0x9B, 0xC7,
            0x0A, 0x74, 0xF8, 0x22, 0x04, 0x29, 0xBA, 0x1D,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
            0xBA, 0xAE, 0xDC, 0xE6, 0xAF, 0x48, 0xA0, 0x3B,
            0xBF, 0xD2, 0x5E, 0x8C, 0xD0, 0x36, 0x41, 0x41
        };
        test_schnorrsig_bip_vectors_check_verify(scratch, pk16, msg16, sig16, 0);
    }
}

/* Nonce function that returns constant 0 */
static int nonce_function_failing(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int counter) {
    (void) msg32;
    (void) key32;
    (void) algo16;
    (void) data;
    (void) counter;
    (void) nonce32;
    return 0;
}

/* Nonce function that sets nonce to 0 */
static int nonce_function_0(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int counter) {
    (void) msg32;
    (void) key32;
    (void) algo16;
    (void) data;
    (void) counter;

    memset(nonce32, 0, 32);
    return 1;
}

void test_schnorrsig_sign(void) {
    unsigned char sk[32];
    const unsigned char msg[32] = "this is a msg for a schnorrsig..";
    secp256k1_schnorrsig sig;

    memset(sk, 23, sizeof(sk));
    CHECK(secp256k1_schnorrsig_sign(ctx, &sig, NULL, msg, sk, NULL, NULL, NULL) == 1);

    /* Overflowing secret key */
    memset(sk, 0xFF, sizeof(sk));
    CHECK(secp256k1_schnorrsig_sign(ctx, &sig, NULL, msg, sk, NULL, NULL, NULL) == 0);
    memset(sk, 23, sizeof(sk));

    CHECK(secp256k1_schnorrsig_sign(ctx, &sig, NULL, msg, sk, NULL, nonce_function_failing, NULL) == 0);
    CHECK(secp256k1_schnorrsig_sign(ctx, &sig, NULL, msg, sk, NULL, nonce_function_0, NULL) == 0);
}

#define N_SIGS  200
/* Creates N_SIGS valid signatures and verifies them with verify and verify_batch. Then flips some
 * bits and checks that verification now fails. */
void test_schnorrsig_sign_verify(secp256k1_scratch_space *scratch) {
    const unsigned char sk[32] = "shhhhhhhh! this key is a secret.";
    unsigned char msg[N_SIGS][32];
    secp256k1_schnorrsig sig[N_SIGS];
    size_t i;
    const secp256k1_schnorrsig *sig_arr[N_SIGS];
    const unsigned char *msg_arr[N_SIGS];
    const secp256k1_pubkey *pk_arr[N_SIGS];
    secp256k1_pubkey pk;

    CHECK(secp256k1_ec_pubkey_create(ctx, &pk, sk));

    CHECK(secp256k1_schnorrsig_verify_batch(ctx, scratch, NULL, NULL, NULL, 0));

    for (i = 0; i < N_SIGS; i++) {
        secp256k1_rand256(msg[i]);
        CHECK(secp256k1_schnorrsig_sign(ctx, &sig[i], NULL, msg[i], sk, NULL, NULL, NULL));
        CHECK(secp256k1_schnorrsig_verify(ctx, &sig[i], msg[i], &pk));
        sig_arr[i] = &sig[i];
        msg_arr[i] = msg[i];
        pk_arr[i] = &pk;
    }

    CHECK(secp256k1_schnorrsig_verify_batch(ctx, scratch, sig_arr, msg_arr, pk_arr, 1));
    CHECK(secp256k1_schnorrsig_verify_batch(ctx, scratch, sig_arr, msg_arr, pk_arr, 2));
    CHECK(secp256k1_schnorrsig_verify_batch(ctx, scratch, sig_arr, msg_arr, pk_arr, 4));
    CHECK(secp256k1_schnorrsig_verify_batch(ctx, scratch, sig_arr, msg_arr, pk_arr, N_SIGS));

    {
        /* Flip a few bits in the signature and in the message and check that
         * verify and verify_batch fail */
        size_t sig_idx = secp256k1_rand_int(4);
        size_t byte_idx = secp256k1_rand_int(32);
        unsigned char xorbyte = secp256k1_rand_int(254)+1;
        sig[sig_idx].data[byte_idx] ^= xorbyte;
        CHECK(!secp256k1_schnorrsig_verify(ctx, &sig[sig_idx], msg[sig_idx], &pk));
        CHECK(!secp256k1_schnorrsig_verify_batch(ctx, scratch, sig_arr, msg_arr, pk_arr, 4));
        sig[sig_idx].data[byte_idx] ^= xorbyte;

        byte_idx = secp256k1_rand_int(32);
        sig[sig_idx].data[32+byte_idx] ^= xorbyte;
        CHECK(!secp256k1_schnorrsig_verify(ctx, &sig[sig_idx], msg[sig_idx], &pk));
        CHECK(!secp256k1_schnorrsig_verify_batch(ctx, scratch, sig_arr, msg_arr, pk_arr, 4));
        sig[sig_idx].data[32+byte_idx] ^= xorbyte;

        byte_idx = secp256k1_rand_int(32);
        msg[sig_idx][byte_idx] ^= xorbyte;
        CHECK(!secp256k1_schnorrsig_verify(ctx, &sig[sig_idx], msg[sig_idx], &pk));
        CHECK(!secp256k1_schnorrsig_verify_batch(ctx, scratch, sig_arr, msg_arr, pk_arr, 4));
        msg[sig_idx][byte_idx] ^= xorbyte;

        /* Check that above bitflips have been reversed correctly */
        CHECK(secp256k1_schnorrsig_verify(ctx, &sig[sig_idx], msg[sig_idx], &pk));
        CHECK(secp256k1_schnorrsig_verify_batch(ctx, scratch, sig_arr, msg_arr, pk_arr, 4));
    }
}
#undef N_SIGS

void test_schnorrsig_s2c_commit_verify(void) {
    unsigned char data32[32];
    secp256k1_schnorrsig sig;
    secp256k1_s2c_opening s2c_opening;
    unsigned char msg[32];
    unsigned char sk[32];
    secp256k1_pubkey pk;
    unsigned char noncedata[32];

    secp256k1_rand256(data32);
    secp256k1_rand256(msg);
    secp256k1_rand256(sk);
    secp256k1_rand256(noncedata);
    CHECK(secp256k1_ec_pubkey_create(ctx, &pk, sk) == 1);

    /* Create and verify correct commitment */
    CHECK(secp256k1_schnorrsig_sign(ctx, &sig, &s2c_opening, msg, sk, data32, NULL, noncedata) == 1);
    CHECK(secp256k1_schnorrsig_verify(ctx, &sig, msg, &pk));
    CHECK(secp256k1_schnorrsig_verify_s2c_commit(ctx, &sig, data32, &s2c_opening) == 1);
    {
        /* verify_s2c_commit fails if nonce_is_negated is wrong */
        secp256k1_s2c_opening s2c_opening_tmp;
        s2c_opening_tmp = s2c_opening;
        s2c_opening_tmp.nonce_is_negated = !s2c_opening.nonce_is_negated;
        CHECK(secp256k1_schnorrsig_verify_s2c_commit(ctx, &sig, data32, &s2c_opening_tmp) == 0);
    }
    {
        /* verify_s2c_commit fails if given data does not match committed data */
        unsigned char data32_tmp[32];
        memcpy(data32_tmp, data32, sizeof(data32_tmp));
        data32_tmp[31] ^= 1;
        CHECK(secp256k1_schnorrsig_verify_s2c_commit(ctx, &sig, data32_tmp, &s2c_opening) == 0);
    }
    {
        /* verify_s2c_commit fails if signature does not commit to data */
        secp256k1_schnorrsig sig_tmp;
        sig_tmp = sig;
        secp256k1_rand256(&sig_tmp.data[0]);
        CHECK(secp256k1_schnorrsig_verify_s2c_commit(ctx, &sig_tmp, data32, &s2c_opening) == 0);
    }
    {
        /* A commitment to different data creates a different original_pubnonce
         * (i.e. data is hashed into the nonce) */
        secp256k1_s2c_opening s2c_opening_tmp;
        secp256k1_schnorrsig sig_tmp;
        unsigned char data32_tmp[32];
        unsigned char serialized_nonce[33];
        unsigned char serialized_nonce_tmp[33];
        size_t outputlen = 33;
        secp256k1_rand256(data32_tmp);
        CHECK(secp256k1_schnorrsig_sign(ctx, &sig_tmp, &s2c_opening_tmp, msg, sk, data32_tmp, NULL, NULL) == 1);
        CHECK(secp256k1_schnorrsig_verify(ctx, &sig_tmp, msg, &pk));
        CHECK(secp256k1_schnorrsig_verify_s2c_commit(ctx, &sig_tmp, data32_tmp, &s2c_opening_tmp) == 1);
        secp256k1_ec_pubkey_serialize(ctx, serialized_nonce, &outputlen, &s2c_opening.original_pubnonce, SECP256K1_EC_COMPRESSED);
        secp256k1_ec_pubkey_serialize(ctx, serialized_nonce_tmp, &outputlen, &s2c_opening_tmp.original_pubnonce, SECP256K1_EC_COMPRESSED);
        CHECK(outputlen == 33);
        CHECK(memcmp(serialized_nonce, serialized_nonce_tmp, outputlen) != 0);
    }
}

void test_schnorrsig_anti_nonce_sidechannel(void) {
    unsigned char msg32[32];
    unsigned char key32[32];
    unsigned char rand32[32];
    unsigned char rand_commitment32[32];
    secp256k1_s2c_opening s2c_opening;
    secp256k1_pubkey client_commit;
    secp256k1_schnorrsig sig;

    secp256k1_rand256(msg32);
    secp256k1_rand256(key32);
    secp256k1_rand256(rand32);

    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_commit(ctx, rand_commitment32, rand32) == 1);

    /* Host sends rand_commitment32 to client. */
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_client_commit(ctx, &client_commit, msg32, key32, rand_commitment32) == 1);
    /* Client sends client_commit to host. Host replies with rand32. */
    CHECK(secp256k1_schnorrsig_sign(ctx, &sig, &s2c_opening, msg32, key32, rand32, NULL, NULL) == 1);
    /* Client sends signature to host. */
    CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_verify(ctx, &sig, rand32, &s2c_opening, &client_commit) == 1);

    {
        /* Signature without commitment to randomness fails verification */
        secp256k1_schnorrsig sig_tmp;
        CHECK(secp256k1_schnorrsig_sign(ctx, &sig_tmp, NULL, msg32, key32, NULL, NULL, NULL) == 1);
        CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_verify(ctx, &sig_tmp, rand32, &s2c_opening, &client_commit) == 0);
    }
    {
        /* If sign-to-contract opening doesn't match commitment, verification fails */
        secp256k1_schnorrsig sig_tmp;
        secp256k1_s2c_opening s2c_opening_tmp;
        unsigned char rand32_tmp[32];
        secp256k1_rand256(rand32_tmp);
        CHECK(secp256k1_schnorrsig_sign(ctx, &sig_tmp, &s2c_opening_tmp, msg32, key32, rand32_tmp, NULL, NULL) == 1);
        CHECK(secp256k1_schnorrsig_anti_nonce_sidechan_host_verify(ctx, &sig_tmp, rand32, &s2c_opening_tmp, &client_commit) == 0);
    }
}

void run_schnorrsig_tests(void) {
    int i;
    secp256k1_scratch_space *scratch = secp256k1_scratch_space_create(ctx, 1024 * 1024);

    test_schnorrsig_serialize();
    test_schnorrsig_api(scratch);
    test_schnorrsig_bip_vectors(scratch);
    test_schnorrsig_sign();
    test_schnorrsig_sign_verify(scratch);
    for (i = 0; i < count; i++) {
        /* Run multiple times to increase probability that the nonce is negated in
         * a test. */
        test_schnorrsig_s2c_commit_verify();
    }
    test_schnorrsig_anti_nonce_sidechannel();

    secp256k1_scratch_space_destroy(scratch);
}

#endif

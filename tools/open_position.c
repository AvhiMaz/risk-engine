#include "../engine/keypair.h"
#include "../engine/rpc.h"
#include "../thirdparty/libbase58/libbase58.h"
#include <curl/curl.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROGRAM_ID_STR "EJ1LFYX1rcpAjXVLptpEUAmehyB4pMbfiUD98r9dfLkZ"

static const uint8_t SYSTEM_PROGRAM[32] = {0};

/*
 * Check if 32 bytes represent a point on the Ed25519 curve.
 * A point is on the curve if x^2 = (y^2 - 1) / (d*y^2 + 1) mod p
 * has a solution. We test using the Legendre symbol.
 *
 * p = 2^255 - 19
 * d = -121665/121666 mod p
 */
static int is_on_ed25519_curve(const uint8_t bytes[32]) {
    BN_CTX *ctx = BN_CTX_new();
    BN_CTX_start(ctx);

    BIGNUM *p = BN_CTX_get(ctx);
    BIGNUM *y = BN_CTX_get(ctx);
    BIGNUM *y2 = BN_CTX_get(ctx);
    BIGNUM *u = BN_CTX_get(ctx);
    BIGNUM *v = BN_CTX_get(ctx);
    BIGNUM *d = BN_CTX_get(ctx);
    BIGNUM *x2 = BN_CTX_get(ctx);
    BIGNUM *exp = BN_CTX_get(ctx);
    BIGNUM *check = BN_CTX_get(ctx);

    BN_set_word(p, 1);
    BN_lshift(p, p, 255);
    BN_sub_word(p, 19);

    uint8_t y_le[32], y_be[32];
    memcpy(y_le, bytes, 32);
    y_le[31] &= 0x7F;
    for (int i = 0; i < 32; i++)
        y_be[i] = y_le[31 - i];
    BN_bin2bn(y_be, 32, y);

    static const uint8_t d_le[32] = {
        0xa3, 0x78, 0x59, 0x13, 0xca, 0x4d, 0xeb, 0x75, 0xab, 0xd8, 0x41,
        0x41, 0x4d, 0x0a, 0x70, 0x00, 0x98, 0xe8, 0x79, 0x77, 0x79, 0x40,
        0xc7, 0x8c, 0x73, 0xfe, 0x6f, 0x2b, 0xee, 0x6c, 0x03, 0x52};
    uint8_t d_be[32];
    for (int i = 0; i < 32; i++)
        d_be[i] = d_le[31 - i];
    BN_bin2bn(d_be, 32, d);

    BN_mod_sqr(y2, y, p, ctx);
    BN_mod_sub(u, y2, BN_value_one(), p, ctx);
    BN_mod_mul(v, d, y2, p, ctx);
    BN_mod_add(v, v, BN_value_one(), p, ctx);
    BN_mod_inverse(v, v, p, ctx);
    BN_mod_mul(x2, u, v, p, ctx);
    BN_sub(exp, p, BN_value_one());
    BN_rshift1(exp, exp);
    BN_mod_exp(check, x2, exp, p, ctx);

    int on_curve = BN_is_one(check) || BN_is_zero(x2);

    BN_CTX_end(ctx);
    BN_CTX_free(ctx);

    return on_curve;
}

/*
 * Derive a program-derived address for the position account.
 * Seeds: "position" || trader[32] || market_index[4] || nonce[8] || bump[1]
 */
static int find_program_address(const uint8_t trader[32], uint32_t market_index,
                                uint64_t nonce, const uint8_t program_id[32],
                                uint8_t pubkey[32], uint8_t *bump) {
    for (int b = 255; b >= 0; b--) {
        uint8_t      hash[32];
        unsigned int hlen = 32;
        uint8_t      bu = (uint8_t)b;

        EVP_MD_CTX  *ctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
        EVP_DigestUpdate(ctx, "position", 8);
        EVP_DigestUpdate(ctx, trader, 32);
        EVP_DigestUpdate(ctx, &market_index, 4);
        EVP_DigestUpdate(ctx, &nonce, 8);
        EVP_DigestUpdate(ctx, &bu, 1);
        EVP_DigestUpdate(ctx, program_id, 32);
        EVP_DigestUpdate(ctx, "ProgramDerivedAddress", 21);
        EVP_DigestFinal_ex(ctx, hash, &hlen);
        EVP_MD_CTX_free(ctx);

        if (!is_on_ed25519_curve(hash)) {
            memcpy(pubkey, hash, 32);
            *bump = bu;
            return 0;
        }
    }
    return -1;
}

int main(void) {
    curl_global_init(CURL_GLOBAL_ALL);

    char kp_path[256];
    snprintf(kp_path, sizeof(kp_path), "%s/.config/solana/id.json",
             getenv("HOME"));

    Keypair kp;
    if (load_keypair(&kp, kp_path) < 0) {
        fprintf(stderr, "failed to load keypair from %s\n", kp_path);
        return 1;
    }

    uint8_t program_id[32];
    size_t  sz = 32;
    b58tobin(program_id, &sz, PROGRAM_ID_STR, 0);

    uint32_t market_index = 0;
    uint64_t nonce = 10;
    uint64_t entry_price = 753450;
    uint64_t size = 10;
    uint64_t margin = 100;
    uint8_t  side = 0; /* LONG */
    uint64_t imr = 1000;
    uint64_t mmr = 500;

    uint8_t  position_pubkey[32];
    uint8_t  bump;
    if (find_program_address(kp.pubkey, market_index, nonce, program_id,
                             position_pubkey, &bump) < 0) {
        fprintf(stderr, "failed to find PDA\n");
        return 1;
    }

    uint8_t tx[512];
    int     off = 0;

    tx[off++] = 1;
    uint8_t *sig = tx + off;
    memset(sig, 0, 64);
    off += 64;

    uint8_t *msg = tx + off;

    tx[off++] = 1; /* num_required_signatures */
    tx[off++] = 0; /* num_readonly_signed */
    tx[off++] = 2; /* num_readonly_unsigned: system_program + program */

    tx[off++] = 4;
    memcpy(tx + off, kp.pubkey, 32);
    off += 32; /* [0] trader */
    memcpy(tx + off, position_pubkey, 32);
    off += 32; /* [1] position */
    memcpy(tx + off, SYSTEM_PROGRAM, 32);
    off += 32; /* [2] system_program */
    memcpy(tx + off, program_id, 32);
    off += 32; /* [3] program */

    uint8_t blockhash[32];
    if (rpc_get_latest_blockhash(blockhash) < 0) {
        fprintf(stderr, "failed to get blockhash\n");
        return 1;
    }
    memcpy(tx + off, blockhash, 32);
    off += 32;

    tx[off++] = 1;  /* num_instructions */
    tx[off++] = 3;  /* program index */
    tx[off++] = 3;  /* num_account_indices */
    tx[off++] = 0;  /* trader */
    tx[off++] = 1;  /* position */
    tx[off++] = 2;  /* system_program */
    tx[off++] = 55; /* data_len */
    tx[off++] = 0;  /* discriminator: open_position */
    memcpy(tx + off, &entry_price, 8);
    off += 8;
    memcpy(tx + off, &size, 8);
    off += 8;
    memcpy(tx + off, &margin, 8);
    off += 8;
    tx[off++] = side;
    memcpy(tx + off, &market_index, 4);
    off += 4;
    memcpy(tx + off, &nonce, 8);
    off += 8;
    tx[off++] = bump;
    memcpy(tx + off, &imr, 8);
    off += 8;
    memcpy(tx + off, &mmr, 8);
    off += 8;

    int       msg_len = (tx + off) - msg;
    EVP_PKEY *pkey =
        EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, kp.secret, 32);
    EVP_MD_CTX *ectx = EVP_MD_CTX_new();
    size_t      sig_len = 64;
    EVP_DigestSignInit(ectx, NULL, NULL, NULL, pkey);
    EVP_DigestSign(ectx, sig, &sig_len, msg, msg_len);
    EVP_MD_CTX_free(ectx);
    EVP_PKEY_free(pkey);

    char out_b64[2048];
    int  encoded = EVP_EncodeBlock((uint8_t *)out_b64, tx, off);
    out_b64[encoded] = '\0';

    rpc_send_transaction(out_b64);

    return 0;
}

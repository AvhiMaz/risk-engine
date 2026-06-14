#include "tx.h"
#include <openssl/evp.h>
#include <stdint.h>
#include <string.h>

/*
 * Solana legacy transaction layout:
 *
 * [1 byte]   number of signatures = 1
 * [64 bytes] signature slot (zeroed, filled after signing)
 *
 * message (signed region):
 * [1 byte]   num_required_signatures = 1
 * [1 byte]   num_readonly_signed = 0
 * [1 byte]   num_readonly_unsigned = 1 (program)
 * [1 byte]   num_accounts = 3
 * [32 bytes] liquidator pubkey (signer)
 * [32 bytes] position pubkey (writable)
 * [32 bytes] program_id (readonly)
 * [32 bytes] recent blockhash
 * [1 byte]   num_instructions = 1
 * [1 byte]   program index = 2
 * [1 byte]   num_account_indices = 2
 * [1 byte]   0 (liquidator)
 * [1 byte]   1 (position)
 * [1 byte]   data_len = 9
 * [1 byte]   1 (discriminator: liquidate_position)
 * [8 bytes]  mark_price little-endian
 */

int tx_liquidate(const uint8_t program_id[32], const uint8_t position[32],
                 const Keypair *kp, uint64_t mark_price,
                 const uint8_t blockhash[32], char *out_b64, int out_cap) {

    uint8_t tx[512];
    int     off = 0;

    tx[off++] = 1;
    uint8_t *sig = tx + off;
    memset(sig, 0, 64);
    off += 64;

    uint8_t *msg = tx + off;

    tx[off++] = 1;
    tx[off++] = 0;
    tx[off++] = 1;

    tx[off++] = 3;
    memcpy(tx + off, kp->pubkey, 32);
    off += 32;
    memcpy(tx + off, position, 32);
    off += 32;
    memcpy(tx + off, program_id, 32);
    off += 32;

    memcpy(tx + off, blockhash, 32);
    off += 32;

    tx[off++] = 1;
    tx[off++] = 2;
    tx[off++] = 2;
    tx[off++] = 0;
    tx[off++] = 1;
    tx[off++] = 9;
    tx[off++] = 1;
    memcpy(tx + off, &mark_price, 8);
    off += 8;

    int       msg_len = (tx + off) - msg;
    EVP_PKEY *pkey =
        EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, kp->secret, 32);
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    size_t      sig_len = 64;
    EVP_DigestSignInit(ctx, NULL, NULL, NULL, pkey);
    EVP_DigestSign(ctx, sig, &sig_len, msg, msg_len);
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    int encoded = EVP_EncodeBlock((uint8_t *)out_b64, tx, off);
    if (encoded >= out_cap)
        return -1;
    out_b64[encoded] = '\0';
    return encoded;
}

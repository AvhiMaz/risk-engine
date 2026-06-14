#ifndef KEYPAIR_H
#define KEYPAIR_H

#include <stdint.h>

typedef struct {
    uint8_t secret[32];
    uint8_t pubkey[32];
} Keypair;

int load_keypair(Keypair *kp, const char *path);

#endif

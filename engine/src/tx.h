#ifndef TX_H
#define TX_H

#include "keypair.h"
#include <stdint.h>

int tx_liquidate(const uint8_t program_id[32], const uint8_t position[32],
                 const Keypair *kp, uint64_t mark_price,
                 const uint8_t blockhash[32], char *out_b64, int out_cap);

#endif

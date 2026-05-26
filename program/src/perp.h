#ifndef PERP_H
#define PERP_H

#include "state.h"
#include <caravel.h>

typedef struct __attribute__((packed)) {
    uint64_t entry_price;
    uint64_t size;
    uint64_t margin;
    uint8_t  side;
    uint32_t market_index;
    uint64_t nonce;
} open_position_args_t;

#define OPEN_POSITION_ACCOUNTS(X)                                              \
    X(trader, SIGNER | WRITABLE)                                               \
    X(position, WRITABLE)                                                      \
    X(system_program, PROGRAM)

IX(0, open_position, OPEN_POSITION_ACCOUNTS, open_position_args_t)

uint64_t open_position(open_position_accounts_t *ctx,
                       open_position_args_t *args, Parameters *params);

#endif

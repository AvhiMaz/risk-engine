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
    uint8_t  bump;
    uint64_t imr;
    uint64_t mmr;
} open_position_args_t;

#define OPEN_POSITION_ACCOUNTS(X)                                              \
    X(trader, SIGNER | WRITABLE)                                               \
    X(position, WRITABLE)                                                      \
    X(system_program, PROGRAM)

IX(0, open_position, OPEN_POSITION_ACCOUNTS, open_position_args_t)

typedef struct __attribute__((packed)) {
    uint64_t mark_price;
} liquidate_position_args_t;

#define LIQUIDATE_POSITION_ACCOUNTS(X)                                         \
    X(liquidator, SIGNER)                                                      \
    X(position, WRITABLE)

IX(1, liquidate_position, LIQUIDATE_POSITION_ACCOUNTS,
   liquidate_position_args_t)

uint64_t open_position(open_position_accounts_t *ctx,
                       open_position_args_t *args, Parameters *params);
uint64_t liquidate_position(liquidate_position_accounts_t *ctx,
                            liquidate_position_args_t     *args,
                            Parameters                    *params);

#endif

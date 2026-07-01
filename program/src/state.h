#ifndef STATE_H
#define STATE_H

#include <caravel.h>

typedef struct {
    Pubkey   trader;
    uint64_t entry_price;
    uint64_t size;
    uint64_t margin;
    uint64_t liquidation_price;
    uint8_t  side;
    uint8_t  state;
    uint32_t market_index;
    uint64_t opened_at;
    uint64_t nonce;
    uint64_t mmr;
} PositionState;

#endif

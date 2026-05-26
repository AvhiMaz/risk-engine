#ifndef STATE_H
#define STATE_H

#include <caravel.h>

typedef struct {
    Pubkey   trader;
    uint64_t entry_price;
    uint64_t size;
    uint64_t margin;
    uint64_t liquidation_price;
    uint8_t  side;  // 0 = LONG, 1 = SHORT
    uint8_t  state; // 0 = OPEN, 1 = LIQUIDATING, 2 = CLOSED
    uint32_t market_index;
    uint64_t opened_at;
    uint64_t nonce;
} PositionState;

#endif

#ifndef POSITION_H
#define POSITION_H

#include <stdint.h>

enum Side { LONG, SHORT };

enum State { OPEN, LIQUIDATING, CLOSED };

typedef struct {
    uint64_t   entry_price;
    uint64_t   size;
    enum Side  side;
    uint64_t   margin;
    uint64_t   liquidation_price;
    uint8_t    trader_id[32];
    uint8_t    pubkey[32];
    uint32_t   market_index;
    uint64_t   opened_at;
    enum State state;
} Position;

#endif

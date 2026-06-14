#ifndef MARKET_H
#define MARKET_H

#include <stdint.h>

typedef struct {
    uint32_t market_index;
    uint64_t initial_margin_rate;
    uint64_t maintenance_margin_rate;
    uint64_t max_leverage;
} MarketConfig;

typedef struct {
    uint32_t     market_index;
    uint64_t     mark_price;
    uint64_t     long_open_interest;
    uint64_t     short_open_interest;
    int64_t      funding_rate;
    uint64_t     last_funding_time;
    MarketConfig config;
} Market;

#endif

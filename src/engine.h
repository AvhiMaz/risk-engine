#ifndef RISK_ENGINE_H
#define RISK_ENGINE_H

#include "market.h"
#include "position.h"
#include <stdint.h>

typedef struct {
    Position  *positions;
    uint32_t   position_count;
    uint32_t   position_cap;

    Market    *markets;
    uint32_t   market_count;
    uint32_t   market_cap;

    Position **liq_queue;
    uint32_t   liq_queue_count;
    uint32_t   liq_queue_cap;
} RiskEngine;

#endif

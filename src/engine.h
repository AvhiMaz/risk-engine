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

void engine_init(RiskEngine *engine);
void engine_add_position(RiskEngine *engine, Position *position);
void engine_add_market(RiskEngine *engine, Market *market);

#endif

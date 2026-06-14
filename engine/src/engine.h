#ifndef RISK_ENGINE_H
#define RISK_ENGINE_H

#include "keypair.h"
#include "market.h"
#include "position.h"
#include <pthread.h>
#include <stdint.h>

typedef struct {
    Position       *positions;
    uint32_t        position_count;
    uint32_t        position_cap;

    Market         *markets;
    uint32_t        market_count;
    uint32_t        market_cap;

    uint32_t       *liq_queue;
    uint32_t        liq_queue_count;
    uint32_t        liq_queue_cap;

    pthread_mutex_t lock;
    pthread_cond_t  price_cond;
    pthread_cond_t  liq_cond;
    uint8_t         program_id[32];
    Keypair         liquidator_kp;

} RiskEngine;

void engine_init(RiskEngine *engine);
void engine_add_position(RiskEngine *engine, Position *position);
void engine_add_market(RiskEngine *engine, Market *market);

#endif

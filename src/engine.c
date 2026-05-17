#include "engine.h"
#include "market.h"
#include "position.h"
#include <stdlib.h>

void engine_init(RiskEngine *engine) {
    pthread_mutex_init(&engine->lock, NULL);
    pthread_cond_init(&engine->price_cond, NULL);
    pthread_cond_init(&engine->liq_cond, NULL);

    engine->positions = malloc(1000 * sizeof(Position));
    engine->position_count = 0;
    engine->position_cap = 1000;

    engine->markets = malloc(64 * sizeof(Market));
    engine->market_count = 0;
    engine->market_cap = 64;

    engine->liq_queue = malloc(256 * sizeof(uint32_t));
    engine->liq_queue_count = 0;
    engine->liq_queue_cap = 256;
}

void engine_add_position(RiskEngine *engine, Position *position) {
    if (engine->position_count == engine->position_cap) {
        engine->position_cap *= 2;
        engine->positions =
            realloc(engine->positions, engine->position_cap * sizeof(Position));
    };
    engine->positions[engine->position_count] = *position;
    engine->position_count++;

    if (position->market_index >= engine->market_count)
        return;
    Market *market = &engine->markets[position->market_index];
    if (position->side == LONG) {
        market->long_open_interest += position->size;
    } else {
        market->short_open_interest += position->size;
    }
}

void engine_add_market(RiskEngine *engine, Market *market) {
    if (engine->market_count == engine->market_cap) {
        engine->market_cap *= 2;
        engine->markets =
            realloc(engine->markets, engine->market_cap * sizeof(Market));
    }

    engine->markets[engine->market_count] = *market;
    engine->market_count++;
}

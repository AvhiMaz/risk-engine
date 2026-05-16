#include "engine.h"
#include "market.h"
#include "position.h"
#include <stdlib.h>

void engine_init(RiskEngine *engine) {
    engine->positions = malloc(1000 * sizeof(Position));
    engine->position_count = 0;
    engine->position_cap = 1000;

    engine->markets = malloc(64 * sizeof(Market));
    engine->market_count = 0;
    engine->market_cap = 64;

    engine->liq_queue = malloc(256 * sizeof(Position *));
    engine->liq_queue_count = 0;
    engine->liq_queue_cap = 256;
}

#include "engine.h"
#include "position.h"
#include <inttypes.h>
#include <stdio.h>

void liquidator(RiskEngine *engine) {
    while (engine->liq_queue_count > 0) {
        uint32_t  idx = engine->liq_queue[0];
        Position *pos = &engine->positions[idx];

        printf("liquidating trader: market %u size %" PRIu64 "\n",
               pos->market_index, pos->size);

        pos->state = CLOSED;

        for (uint32_t i = 0; i < engine->liq_queue_count - 1; i++) {
            engine->liq_queue[i] = engine->liq_queue[i + 1];
        }

        engine->liq_queue_count--;
    }
}

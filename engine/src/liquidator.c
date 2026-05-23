#include "engine.h"
#include "position.h"
#include <inttypes.h>
#include <stdio.h>

void *liquidator(void *arg) {
    RiskEngine *engine = (RiskEngine *)arg;

    while (1) {

        pthread_mutex_lock(&engine->lock);
        while (engine->liq_queue_count == 0) {
            pthread_cond_wait(&engine->liq_cond, &engine->lock);
        }

        uint32_t  idx = engine->liq_queue[0];
        Position *pos = &engine->positions[idx];

        printf("liquidating trader: market %u size %" PRIu64 "\n",
               pos->market_index, pos->size);

        pos->state = CLOSED;

        for (uint32_t i = 0; i < engine->liq_queue_count - 1; i++) {
            engine->liq_queue[i] = engine->liq_queue[i + 1];
        }

        engine->liq_queue_count--;
        pthread_mutex_unlock(&engine->lock);
    }

    return NULL;
}

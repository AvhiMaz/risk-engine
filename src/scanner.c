#include "engine.h"
#include "market.h"
#include "position.h"
#include <stdlib.h>

void *scanner(void *arg) {
    RiskEngine *engine = (RiskEngine *)arg;

    while (1) {
        pthread_mutex_lock(&engine->lock);
        pthread_cond_wait(&engine->price_cond, &engine->lock);

        for (uint32_t i = 0; i < engine->position_count; i++) {
            Position *pos = &engine->positions[i];
            if (pos->state != OPEN)
                continue;
            Market *market = &engine->markets[pos->market_index];

            int     liquidate = 0;
            if (pos->side == LONG &&
                market->mark_price <= pos->liquidation_price)
                liquidate = 1;
            else if (pos->side == SHORT &&
                     market->mark_price >= pos->liquidation_price)
                liquidate = 1;

            if (liquidate) {
                pos->state = LIQUIDATING;

                if (engine->liq_queue_count == engine->liq_queue_cap) {
                    engine->liq_queue_cap *= 2;
                    engine->liq_queue =
                        realloc(engine->liq_queue,
                                engine->liq_queue_cap * sizeof(uint32_t));
                }

                engine->liq_queue[engine->liq_queue_count] = i;
                engine->liq_queue_count++;
                pthread_cond_signal(&engine->liq_cond);
            }
        }
        pthread_mutex_unlock(&engine->lock);
    }
    return NULL;
}

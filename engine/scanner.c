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
            Market  *market = &engine->markets[pos->market_index];

            uint64_t notional = market->mark_price * pos->size / 10000;
            uint64_t maint_req = notional * pos->mmr / 10000;

            int64_t  price_diff =
                (pos->side == LONG)
                     ? (int64_t)market->mark_price - (int64_t)pos->entry_price
                     : (int64_t)pos->entry_price - (int64_t)market->mark_price;

            int64_t upnl = price_diff * (int64_t)pos->size / 10000;
            int64_t eff_margin = (int64_t)pos->margin + upnl;

            int     liquidate = eff_margin < (int64_t)maint_req;

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

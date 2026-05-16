#include "engine.h"
#include "market.h"
#include "position.h"
#include <stdint.h>
#include <stdlib.h>

void scanner(RiskEngine *engine) {
    for (uint32_t i = 0; i < engine->position_count; i++) {
        Position *pos = &engine->positions[i];
        if (pos->state != OPEN)
            continue;
        Market *market = &engine->markets[pos->market_index];

        int64_t unrealized_pnl;

        if (pos->side == LONG) {
            unrealized_pnl =
                ((int64_t)market->mark_price - (int64_t)pos->entry_price) *
                (int64_t)pos->size;
        } else {
            unrealized_pnl =
                ((int64_t)pos->entry_price - (int64_t)market->mark_price) *
                (int64_t)pos->size;
        }

        int64_t equity = (int64_t)pos->margin + unrealized_pnl;

        int64_t position_value =
            (int64_t)market->mark_price * (int64_t)pos->size;
        int64_t health = (equity * 100) / position_value;

        if (health < (int64_t)market->config.maintenance_margin_rate) {
            pos->state = LIQUIDATING;

            if (engine->liq_queue_count == engine->liq_queue_cap) {
                engine->liq_queue_cap *= 2;
                engine->liq_queue =
                    realloc(engine->liq_queue,
                            engine->liq_queue_cap * sizeof(Position *));
            }

            engine->liq_queue[engine->liq_queue_count] = pos;
            engine->liq_queue_count++;
        }
    }
}

#include "engine.h"
#include "market.h"
#include "position.h"
#include <stdint.h>

void scanner(RiskEngine *engine) {
    for (uint32_t i = 0; i < engine->position_count; i++) {
        Position *pos = &engine->positions[i];
        if (pos->state != OPEN)
            continue;
        Market *market = &engine->markets[pos->market_index];

        int64_t unrealized_pnl;

        if (pos->side == LONG) {
            // mark_price - entry_price * size
            unrealized_pnl =
                ((int64_t)market->mark_price - (int64_t)pos->entry_price) *
                (int64_t)pos->size;
        } else {
            // entry_price - mark_price * size
            unrealized_pnl =
                ((int64_t)pos->entry_price - (int64_t)market->mark_price) *
                (int64_t)pos->size;
        }

        int64_t equity = (int64_t)pos->margin + unrealized_pnl;
    }
}

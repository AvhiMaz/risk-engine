#include "engine.h"
#include "liquidator.h"
#include "market.h"
#include "position.h"
#include "scanner.h"
#include <stdio.h>

int main() {
    RiskEngine engine;
    engine_init(&engine);

    Market sol_perp = {.market_index = 0,
                       .mark_price = 100,
                       .config = {.market_index = 0,
                                  .initial_margin_rate = 10,
                                  .maintenance_margin_rate = 5,
                                  .max_leverage = 20}};

    engine_add_market(&engine, &sol_perp);

    Position pos = {.entry_price = 200,
                    .size = 10,
                    .side = LONG,
                    .margin = 100,
                    .market_index = 0,
                    .state = OPEN,
                    .liquidation_price = 180,
                    .opened_at = 0,
                    .trader_id = {0}};

    engine_add_position(&engine, &pos);

    scanner(&engine);
    printf("liq_queue_count: %u\n", engine.liq_queue_count);
    liquidator(&engine);

    return 0;
}

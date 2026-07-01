#include "engine.h"
#include "feed.h"
#include "keypair.h"
#include "liquidator.h"
#include "market.h"
#include "rpc.h"
#include "scanner.h"
#include <curl/curl.h>
#include <stdio.h>

#include "../thirdparty/libbase58/libbase58.h"

#define PROGRAM_ID "EJ1LFYX1rcpAjXVLptpEUAmehyB4pMbfiUD98r9dfLkZ"

int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    RiskEngine engine;
    engine_init(&engine);

    char kp_path[256];
    snprintf(kp_path, sizeof(kp_path), "%s/.config/solana/id.json",
             getenv("HOME"));
    if (load_keypair(&engine.liquidator_kp, kp_path) < 0) {
        fprintf(stderr, "failed to load keypair from %s\n", kp_path);
        return 1;
    }

    size_t sz = 32;
    b58tobin(engine.program_id, &sz, PROGRAM_ID, 0);

    Market sol_perp = {.market_index = 0,
                       .mark_price = 0,
                       .config = {.market_index = 0,
                                  .initial_margin_rate = 1000,
                                  .maintenance_margin_rate = 500,
                                  .max_leverage = 20}};
    engine_add_market(&engine, &sol_perp);

    if (rpc_get_program_accounts(&engine, PROGRAM_ID) < 0)
        fprintf(stderr, "failed to fetch positions\n");

    pthread_t scanner_thread;
    pthread_t liquidator_thread;
    pthread_t feed_thread;

    pthread_create(&scanner_thread, NULL, scanner, &engine);
    pthread_create(&liquidator_thread, NULL, liquidator, &engine);
    pthread_create(&feed_thread, NULL, feed, &engine);

    pthread_join(scanner_thread, NULL);
    pthread_join(liquidator_thread, NULL);
    pthread_join(feed_thread, NULL);

    return 0;
}

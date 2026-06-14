#include "engine.h"
#include "position.h"
#include "rpc.h"
#include "tx.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

void *liquidator(void *arg) {
    RiskEngine *engine = (RiskEngine *)arg;

    while (1) {
        pthread_mutex_lock(&engine->lock);
        while (engine->liq_queue_count == 0)
            pthread_cond_wait(&engine->liq_cond, &engine->lock);

        uint32_t idx       = engine->liq_queue[0];
        Position *pos      = &engine->positions[idx];
        uint64_t mark_price = engine->markets[pos->market_index].mark_price;

        uint8_t pubkey[32];
        memcpy(pubkey, pos->pubkey, 32);
        pos->state = CLOSED;

        for (uint32_t i = 0; i < engine->liq_queue_count - 1; i++)
            engine->liq_queue[i] = engine->liq_queue[i + 1];
        engine->liq_queue_count--;
        pthread_mutex_unlock(&engine->lock);

        uint8_t blockhash[32];
        if (rpc_get_latest_blockhash(blockhash) < 0) {
            fprintf(stderr, "failed to get blockhash\n");
            continue;
        }

        char tx_b64[2048];
        if (tx_liquidate(engine->program_id, pubkey, &engine->liquidator_kp,
                         mark_price, blockhash, tx_b64, sizeof(tx_b64)) < 0) {
            fprintf(stderr, "failed to build tx\n");
            continue;
        }

        rpc_send_transaction(tx_b64);
    }

    return NULL;
}

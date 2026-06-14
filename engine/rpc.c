#include "rpc.h"
#include <curl/curl.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../thirdparty/cjson/cJSON.h"
#include "../thirdparty/libbase58/libbase58.h"
#include "engine.h"

typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} Buffer;

typedef struct {
    uint8_t  trader[32];
    uint64_t entry_price;
    uint64_t size;
    uint64_t margin;
    uint64_t liquidation_price;
    uint8_t  side;
    uint8_t  state;
    uint8_t  _pad[2];
    uint32_t market_index;
    uint64_t opened_at;
    uint64_t nonce;
} OnChainPosition;

/*
 * source: https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
 */
static size_t write_cb(char *data, size_t size, size_t nmemb, void *ud) {
    size_t  realsize = nmemb;
    Buffer *mem = (Buffer *)ud;

    char   *ptr = realloc(mem->data, mem->len + realsize + 1);
    if (!ptr)
        return 0;

    mem->data = ptr;
    memcpy(&(mem->data[mem->len]), data, realsize);
    mem->len += realsize;
    mem->data[mem->len] = 0;

    return realsize;
}

static int http_post(const char *body, Buffer *out) {
    CURL *curl = curl_easy_init();
    if (!curl)
        return -1;

    struct curl_slist *hdrs = NULL;

    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, RPC_URL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));

    return res == CURLE_OK ? 0 : -1;
}

int rpc_get_latest_blockhash(uint8_t blockhash[32]) {

    const char *body =
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"getLatestBlockhash\"}";

    Buffer resp = {malloc(4096), 0, 4096};

    if (http_post(body, &resp) < 0) {
        free(resp.data);
        return -1;
    }

    cJSON *json = cJSON_Parse(resp.data);
    free(resp.data);
    if (!json)
        return -1;

    cJSON *result = cJSON_GetObjectItem(json, "result");
    cJSON *value = cJSON_GetObjectItem(result, "value");
    cJSON *bh = cJSON_GetObjectItem(value, "blockhash");

    if (!bh || !bh->valuestring) {
        cJSON_Delete(json);
        return -1;
    }

    size_t sz = 32;
    b58tobin(blockhash, &sz, bh->valuestring, 0);

    cJSON_Delete(json);

    return 0;
}

int rpc_send_transaction(const char *tx_b64) {
    char body[8192];
    snprintf(body, sizeof(body),
             "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"sendTransaction\","
             "\"params\":[\"%s\",{\"encoding\":\"base64\"}]}",
             tx_b64);

    Buffer resp = {malloc(4096), 0, 4096};

    int    res = http_post(body, &resp);

    if (res == 0) {
        printf("%s\n", resp.data);
    }

    free(resp.data);

    return res;
}

int rpc_get_program_accounts(RiskEngine *engine, const char *program_id) {
    char body[512];
    snprintf(body, sizeof(body),
             "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"getProgramAccounts\","
             "\"params\":[\"%s\",{\"encoding\":\"base64\","
             "\"filters\":[{\"dataSize\":%zu}]}]}",
             program_id, sizeof(OnChainPosition));

    Buffer resp = {malloc(4096), 0, 4096};
    if (http_post(body, &resp) < 0) {
        free(resp.data);
        return -1;
    }

    printf("blockhash response: %s\n", resp.data);

    cJSON *json = cJSON_Parse(resp.data);
    free(resp.data);
    if (!json)
        return -1;

    cJSON *result = cJSON_GetObjectItem(json, "result");
    if (!result) {
        cJSON_Delete(json);
        return -1;
    }

    cJSON *item;
    cJSON_ArrayForEach(item, result) {
        cJSON *pubkey_str = cJSON_GetObjectItem(item, "pubkey");
        cJSON *account = cJSON_GetObjectItem(item, "account");
        cJSON *data_arr = cJSON_GetObjectItem(account, "data");
        cJSON *data_b64 = cJSON_GetArrayItem(data_arr, 0);

        if (!pubkey_str || !data_b64)
            continue;

        uint8_t raw[256];
        int     n = EVP_DecodeBlock(raw, (uint8_t *)data_b64->valuestring,
                                    strlen(data_b64->valuestring));
        if (n < (int)sizeof(OnChainPosition))
            continue;

        OnChainPosition *op = (OnChainPosition *)raw;
        if (op->state != 0)
            continue;

        Position pos = {0};
        pos.entry_price = op->entry_price;
        pos.size = op->size;
        pos.margin = op->margin;
        pos.liquidation_price = op->liquidation_price;
        pos.side = op->side == 0 ? LONG : SHORT;
        pos.market_index = op->market_index;
        pos.opened_at = op->opened_at;
        pos.state = OPEN;
        memcpy(pos.trader_id, op->trader, 32);

        size_t sz = 32;
        b58tobin(pos.pubkey, &sz, pubkey_str->valuestring, 0);

        pthread_mutex_lock(&engine->lock);
        engine_add_position(engine, &pos);
        pthread_mutex_unlock(&engine->lock);
    }

    cJSON_Delete(json);
    return 0;
}

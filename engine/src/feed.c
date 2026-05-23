#include "../thirdparty/cjson/cJSON.h"
#include "engine.h"
#include <libwebsockets.h>
#include <stdio.h>
#include <string.h>

static int callback_pyth(struct lws *wsi, enum lws_callback_reasons reason,
                         void *user, void *in, size_t len) {

    RiskEngine *engine = lws_context_user(lws_get_context(wsi));

    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        printf("connected to pyth\n");
        lws_callback_on_writable(wsi);
        break;

    case LWS_CALLBACK_CLIENT_WRITEABLE: {
        unsigned char buf[LWS_PRE + 256];
        const char   *msg = "{\"type\":\"subscribe\",\"ids\":"
                            "[\"0xef0d8b6fda2ceba41da15d4095d1da392a0d2f8ed0c6c7b"
                            "c0f4cfac8c280b56d\"]}";
        int           msg_len = strlen(msg);
        memcpy(&buf[LWS_PRE], msg, msg_len);
        lws_write(wsi, &buf[LWS_PRE], msg_len, LWS_WRITE_TEXT);
        break;
    }

    case LWS_CALLBACK_CLIENT_RECEIVE: {
        cJSON *json = cJSON_ParseWithLength((char *)in, len);
        if (!json)
            break;

        cJSON *type = cJSON_GetObjectItem(json, "type");
        if (!type || strcmp(type->valuestring, "price_update") != 0) {
            cJSON_Delete(json);
            break;
        }

        cJSON *price_feed = cJSON_GetObjectItem(json, "price_feed");
        cJSON *price_obj = cJSON_GetObjectItem(price_feed, "price");
        cJSON *price_val = cJSON_GetObjectItem(price_obj, "price");
        cJSON *expo_val = cJSON_GetObjectItem(price_obj, "expo");

        if (!price_val || !expo_val) {
            cJSON_Delete(json);
            break;
        }

        long long raw_price = atoll(price_val->valuestring);
        int       expo = expo_val->valueint;

        uint64_t  mark_price = raw_price;
        for (int i = 0; i < -expo - 4; i++)
            mark_price /= 10;

        pthread_mutex_lock(&engine->lock);
        engine->markets[0].mark_price = mark_price;
        pthread_cond_signal(&engine->price_cond);
        pthread_mutex_unlock(&engine->lock);

        printf("SOL mark_price: $%" PRIu64 ".%04" PRIu64 "\n",
               mark_price / 10000, mark_price % 10000);

        cJSON_Delete(json);
        break;
    }

    case LWS_CALLBACK_CLIENT_CLOSED:
        printf("disconnected\n");
        break;

    default:
        break;
    }

    return 0;
};

void *feed(void *arg) {

    RiskEngine                       *engine = (RiskEngine *)arg;

    static const struct lws_protocols protocols[] = {
        {"pyth", callback_pyth, 0, 0, 0, NULL, 0}, LWS_PROTOCOL_LIST_TERM};

    struct lws_context_creation_info info = {0};

    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.user = engine;

    struct lws_context *ctx = lws_create_context(&info);

    if (!ctx) {
        printf("failed to create context\n");
        return NULL;
    }

    struct lws_client_connect_info conn = {0};

    conn.context = ctx;
    conn.address = "hermes.pyth.network";
    conn.port = 443;
    conn.path = "/ws";
    conn.host = "hermes.pyth.network";
    conn.origin = "hermes.pyth.network";
    conn.ssl_connection = LCCSCF_USE_SSL;
    conn.protocol = "pyth";

    lws_client_connect_via_info(&conn);

    while (1) {
        lws_service(ctx, 0);
    }

    return NULL;
}

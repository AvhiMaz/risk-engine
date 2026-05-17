#ifndef FEED_H
#define FEED_H

#include <libwebsockets.h>
#include <stdio.h>
#include <string.h>

void      *feed(void *arg);
static int callback_pyth(struct lws *wsi, enum lws_callback_reasons reason,
                         void *user, void *in, size_t len);

#endif

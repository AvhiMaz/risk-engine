#include "perp.h"

static uint64_t process(Parameters *params) {
    DISPATCH(params, HANDLER(open_position) HANDLER(liquidate_position));
    return SUCCESS;
}

LEGACY_ENTRYPOINT(process)

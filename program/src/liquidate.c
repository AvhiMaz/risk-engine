#include "perp.h"

uint64_t liquidate_position(liquidate_position_accounts_t *ctx,
                            liquidate_position_args_t     *args,
                            Parameters                    *params) {

    if (!pubkey_eq(ACCOUNT_OWNER(ctx->position), params->program_id))
        return ERROR_INVALID_ARGUMENT;

    PositionState *state = ACCOUNT_STATE(ctx->position, PositionState);

    if (state->state != 0) {
        return ERROR_INVALID_ARGUMENT;
    }

    uint64_t notional = args->mark_price * state->size / 10000;
    uint64_t maint_req = notional * state->mmr / 10000;

    int64_t  price_diff =
        (state->side == 0)
             ? (int64_t)args->mark_price - (int64_t)state->entry_price
             : (int64_t)state->entry_price - (int64_t)args->mark_price;

    uint64_t abs_diff = (uint64_t)(price_diff < 0 ? -price_diff : price_diff);
    uint64_t abs_upnl = abs_diff * state->size / 10000;
    int64_t  upnl = price_diff < 0 ? -(int64_t)abs_upnl : (int64_t)abs_upnl;
    int64_t  eff_margin = (int64_t)state->margin + upnl;

    if (eff_margin >= (int64_t)maint_req)
        return ERROR_INVALID_ARGUMENT;

    state->state = 2;

    return SUCCESS;
}

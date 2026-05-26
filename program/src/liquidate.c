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

    if (state->side == 0) {
        if (args->mark_price > state->liquidation_price)
            return ERROR_INVALID_ARGUMENT;
    } else {
        if (args->mark_price < state->liquidation_price)
            return ERROR_INVALID_ARGUMENT;
    }

    state->state = 2;

    return SUCCESS;
}

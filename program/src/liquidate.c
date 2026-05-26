#include "perp.h"

uint64_t liquidate_position(liquidate_position_accounts_t *ctx,
                            liquidate_position_args_t     *args,
                            Parameters                    *params) {

    PositionState *state = ACCOUNT_STATE(ctx->position, PositionState);

    SignerSeed     derive[] = {
            SEED_STR("position"),
            SEED_PUBKEY(&state->trader),
            SEED_BYTES(&state->market_index, sizeof(uint32_t)),
            SEED_BYTES(&state->nonce, sizeof(uint64_t)),
    };

    Pubkey  expected;
    uint8_t bump;
    find_program_address(derive, 4, params->program_id, &expected, &bump);

    if (!pubkey_eq(ctx->position->key, &expected)) {
        return ERROR_INVALID_PDA;
    }

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

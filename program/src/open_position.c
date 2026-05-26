#include "perp.h"

uint64_t open_position(open_position_accounts_t *ctx,
                       open_position_args_t *args, Parameters *params) {
    Rent     rent;
    uint64_t lamports;
    get_rent(&rent);
    lamports = minimum_balance(&rent, sizeof(PositionState));

    SignerSeed seeds[] = {
        SEED_STR("position"),
        SEED_PUBKEY(ctx->trader->key),
        SEED_BYTES(&args->market_index, sizeof(uint32_t)),
        SEED_BYTES(&args->nonce, sizeof(uint64_t)),
        SEED_U8(&args->bump),
    };
    SignerSeeds signer = {.seeds = seeds, .len = 5};

    TRY(system_create_account_signed(ctx->trader, ctx->position, lamports,
                                     sizeof(PositionState), params->program_id,
                                     params->accounts, params->accounts_len,
                                     &signer, 1));

    Clock clock;
    get_clock(&clock);

    PositionState *state = ACCOUNT_STATE(ctx->position, PositionState);
    state->entry_price       = args->entry_price;
    state->size              = args->size;
    state->margin            = args->margin;
    state->side              = args->side;
    state->state             = 0;
    state->opened_at         = clock.unix_timestamp;
    state->liquidation_price = (args->side == 0)
        ? args->entry_price - (args->margin / args->size)
        : args->entry_price + (args->margin / args->size);
    state->market_index = args->market_index;
    state->nonce        = args->nonce;
    pubkey_cpy(&state->trader, ctx->trader->key);

    return SUCCESS;
}

#ifndef RPC_H
#define RPC_H

#include "engine.h"
#include <stdint.h>
#include <stdlib.h>

#define RPC_URL "https://api.devnet.solana.com"

int rpc_get_latest_blockhash(uint8_t blockhash[32]);
int rpc_send_transaction(const char *tx_b64);
int rpc_get_program_accounts(RiskEngine *engine, const char *program_id);

#endif

use mollusk_svm::Mollusk;
use solana_account::AccountSharedData;
use solana_instruction::{AccountMeta, Instruction};
use solana_pubkey::Pubkey;

const PROGRAM_ID: Pubkey = Pubkey::new_from_array([1u8; 32]);
const SYSTEM_PROGRAM_ID: Pubkey = Pubkey::new_from_array([0u8; 32]);

#[test]
fn test_open_position() {
    let mollusk = Mollusk::new(&PROGRAM_ID, "../build/perps");

    let trader = Pubkey::new_unique();
    let market_index: u32 = 0;
    let nonce: u64 = 0;

    let (position, _bump) = Pubkey::find_program_address(
        &[
            b"position",
            trader.as_ref(),
            &market_index.to_le_bytes(),
            &nonce.to_le_bytes(),
        ],
        &PROGRAM_ID,
    );

    let mut data = vec![0u8];
    data.extend_from_slice(&100u64.to_le_bytes());
    data.extend_from_slice(&10u64.to_le_bytes());
    data.extend_from_slice(&50u64.to_le_bytes());
    data.push(0u8);
    data.extend_from_slice(&market_index.to_le_bytes());
    data.extend_from_slice(&nonce.to_le_bytes());

    let instruction = Instruction::new_with_bytes(
        PROGRAM_ID,
        &data,
        vec![
            AccountMeta::new(trader, true),
            AccountMeta::new(position, false),
            AccountMeta::new_readonly(SYSTEM_PROGRAM_ID, false),
        ],
    );

    let trader_account = AccountSharedData::new(1_000_000_000, 0, &SYSTEM_PROGRAM_ID);
    let position_account = AccountSharedData::default();

    let result = mollusk.process_instruction(
        &instruction,
        &[
            (trader, trader_account),
            (position, position_account),
            mollusk_svm::program::keyed_account_for_system_program(),
        ],
    );

    assert!(!result.program_result.is_err());
}

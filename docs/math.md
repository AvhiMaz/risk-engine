# Math Specification

## Units and Scale

All prices are `uint64` scaled to **4 decimal places** (1 unit = $0.0001).  
All margin rates are `uint64` in **basis points** (1 bp = 0.01%, 10000 bp = 100%).  
All position sizes and margins are `uint64` in raw units.

| Constant      | Value | Meaning                  |
| ------------- | ----- | ------------------------ |
| `PRICE_SCALE` | 10000 | 4 decimal places         |
| `BPS_DENOM`   | 10000 | basis point denominator  |

---

## Liquidation Price

The liquidation price is computed at position open using the initial margin rate (IMR) and maintenance margin rate (MMR) from the market config.

**LONG:**

```
liq_price = entry_price * (BPS_DENOM - IMR + MMR) / BPS_DENOM
```

**SHORT:**

```
liq_price = entry_price * (BPS_DENOM + IMR - MMR) / BPS_DENOM
```

Example with `entry_price = 682900`, `IMR = 1000` (10%), `MMR = 500` (5%):

```
LONG:  liq_price = 682900 * (10000 - 1000 + 500) / 10000 = 682900 * 9500 / 10000 = 648755
SHORT: liq_price = 682900 * (10000 + 1000 - 500) / 10000 = 682900 * 10500 / 10000 = 717045
```

Intermediate `entry_price * factor` fits in `uint64` for realistic price values (up to ~$1M scaled = 10^10, factor <= 20000, product < 2^47).

---

## Unrealized PnL

**LONG:**

```
upnl = (mark_price - entry_price) * size / PRICE_SCALE
```

**SHORT:**

```
upnl = (entry_price - mark_price) * size / PRICE_SCALE
```

`upnl` is a signed `int64`. Negative means the position is losing.

Division by `PRICE_SCALE` keeps upnl in the same units as `margin` so the health check comparison is valid.

---

## Liquidation Condition

The health check is recomputed dynamically at liquidation time using the current mark price:

```
notional        = mark_price * size / PRICE_SCALE
price_diff      = mark_price - entry_price          // LONG (signed)
price_diff      = entry_price - mark_price          // SHORT (signed)
upnl            = price_diff * size / PRICE_SCALE
effective_margin = margin + upnl
maintenance_req  = notional * MMR / BPS_DENOM

liquidatable = effective_margin < maintenance_req
```

The static `liq_price` stored at open is an approximation for display purposes only. Health is always recomputed dynamically at liquidation time.

---

## Integer Arithmetic

All math uses `int64_t` and `uint64_t`. No 128-bit types are used because the SBF target does not provide the required compiler-rt builtins (`__divdi3`, `__udivti3`).

To avoid signed division on the SBF target (which requires `__divdi3`), upnl is computed via the absolute value trick:

```c
uint64_t notional  = mark_price * size / 10000;
uint64_t maint_req = notional * mmr / 10000;

int64_t price_diff = (side == LONG)
    ? (int64_t)mark_price - (int64_t)entry_price
    : (int64_t)entry_price - (int64_t)mark_price;

uint64_t abs_diff  = (uint64_t)(price_diff < 0 ? -price_diff : price_diff);
uint64_t abs_upnl  = abs_diff * size / 10000;
int64_t  upnl      = price_diff < 0 ? -(int64_t)abs_upnl : (int64_t)abs_upnl;
int64_t  eff_margin = (int64_t)margin + upnl;

liquidatable = eff_margin < (int64_t)maint_req;
```

The off-chain engine (compiled with GCC for the host) uses direct signed division since `__divdi3` is available there.

---

## Market Config

Each market has a config stored in the engine:

| Field                     | Type | Unit         | Example    |
| ------------------------- | ---- | ------------ | ---------- |
| `market_index`            | u32  |              | 0          |
| `initial_margin_rate`     | u64  | basis points | 1000 (10%) |
| `maintenance_margin_rate` | u64  | basis points | 500 (5%)   |
| `max_leverage`            | u64  | integer      | 20         |

---

## Funding Rate

Not yet implemented. Planned:

```
premium         = mark_price - index_price
funding_rate    = premium / index_price
funding_payment = funding_rate * notional * dt
margin         += funding_payment
```

After funding accrual, margin must be re-checked against MMR.

---

## Summary of Invariants

1. `MMR < IMR` always. maintenance threshold is below initial.
2. A position opened with `IMR` cannot be immediately liquidatable.
3. All division is integer division. no floats anywhere.
4. upnl must be divided by `PRICE_SCALE` to match margin units before adding.
5. Liquidation price stored at open is approximate. always recompute health dynamically at liquidation time.

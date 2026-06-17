# Math Specification

## Units and Scale

All prices are `uint64` scaled to **4 decimal places** (1 unit = $0.0001).  
All margin rates are `uint64` in **basis points** (1 bp = 0.01%, 10000 bp = 100%).  
All position sizes and margins are `uint64` in raw units.

| Constant      | Value | Meaning                             |
| ------------- | ----- | ----------------------------------- |
| `PRICE_SCALE` | 10000 | 4 decimal places                    |
| `BPS_DENOM`   | 10000 | basis point denominator             |
| `WIDE_SCALE`  | 2^64  | intermediate for overflow-safe math |

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

**Overflow safety:** intermediate `entry_price * (BPS_DENOM +/- IMR +/- MMR)` must use `uint128` before dividing.

---

## Unrealized PnL

**LONG:**

```
upnl = (mark_price - entry_price) * size
```

**SHORT:**

```
upnl = (entry_price - mark_price) * size
```

`upnl` is a signed `int64`. Negative means the position is losing.

---

## Margin Ratio

The margin ratio represents the current health of a position.

```
effective_margin = margin + upnl
margin_ratio     = effective_margin * BPS_DENOM / (mark_price * size / PRICE_SCALE)
```

A position is healthy if `margin_ratio >= MMR`.  
A position is liquidatable if `margin_ratio < MMR`.

---

## Liquidation Condition

Rather than comparing against a static liquidation price, the correct health check at liquidation time is:

```
notional       = mark_price * size / PRICE_SCALE
upnl           = (mark_price - entry_price) * size  // LONG
effective_margin = margin + upnl
maintenance_req  = notional * MMR / BPS_DENOM

liquidatable = effective_margin < maintenance_req
```

The static `liq_price` stored at open is an approximation for display purposes. The crank must recompute health dynamically using the current mark price.

---

## Funding Rate

Funding transfers value between longs and shorts based on the divergence between mark price and index price.

```
premium        = mark_price - index_price
funding_rate   = premium / index_price           // as a fraction
funding_payment = funding_rate * notional * dt   // per funding interval
```

- Longs pay funding when `mark_price > index_price` (positive funding rate)
- Shorts pay funding when `mark_price < index_price` (negative funding rate)

Funding is accrued into `margin` on each settle instruction call.

```
margin += funding_payment   // positive = received, negative = paid
```

After funding accrual, margin must be re-checked against MMR. If `margin + upnl < maintenance_req`, the position becomes liquidatable.

---

## Effective Margin After Funding

```
effective_margin = margin + upnl + accrued_funding
```

Where `accrued_funding` accumulates over time via the funding accumulator pattern:

```
accrued_funding = size * (f_now - f_snap) / PRICE_SCALE
```

`f_snap` is recorded at position open or last settlement. `f_now` is the global per-market funding accumulator maintained by the crank.

---

## Wide Math

All intermediate multiplications must use 128-bit integers to prevent overflow.

```c
uint128_t notional = (uint128_t)mark_price * size / PRICE_SCALE;
uint128_t maint_req = notional * mmr / BPS_DENOM;
int128_t  upnl = (int128_t)(mark_price - entry_price) * size;
int128_t  eff_margin = (int128_t)margin + upnl;
```

Final results must fit in `int64` or `uint64` before being stored in account state.

---

## Market Config

Each market has a config stored on-chain:

| Field                     | Type | Unit         | Example    |
| ------------------------- | ---- | ------------ | ---------- |
| `market_index`            | u32  |              | 0          |
| `initial_margin_rate`     | u64  | basis points | 1000 (10%) |
| `maintenance_margin_rate` | u64  | basis points | 500 (5%)   |
| `max_leverage`            | u64  | integer      | 20         |

---

## Summary of Invariants

1. `MMR < IMR` always. maintenance threshold is below initial.
2. A position opened with `IMR` cannot be immediately liquidatable.
3. Effective margin must use `int128` intermediates. never truncate before final result.
4. Funding accrual must happen before any health check.
5. Liquidation price stored at open is approximate. always recompute health dynamically at liquidation time.

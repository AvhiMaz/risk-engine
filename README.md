## risk-engine

a high-performance perpetuals risk engine in pure c, paired with a solana on-chain program also written in c using the caravel sdk. the engine scans open positions, checks health using dynamic margin math, and submits liquidation transactions on-chain in real time.

> ⚠️ **DISCLAIMER:** THE CODE IS UNAUDITED AND NOT PRODUCTION READY.

## math

all math uses integer arithmetic, no floats. prices are scaled to 4 decimal places (`price_scale = 10000`). margin rates are in basis points (`bps_denom = 10000`).

liquidation price at open:

```
long:  liq_price = entry_price * (10000 - imr + mmr) / 10000
short: liq_price = entry_price * (10000 + imr - mmr) / 10000
```

health check at liquidation time:

```
notional   = mark_price * size / 10000
maint_req  = notional * mmr / 10000
upnl       = price_diff * size / 10000
eff_margin = margin + upnl

liquidatable = eff_margin < maint_req
```

full spec in [docs/math.md](docs/math.md).

## credits

- [cJSON](https://github.com/DaveGamble/cJSON) by Dave Gamble, vendored in `thirdparty/`
- [libbase58](https://github.com/luke-jr/libbase58) by Luke Dashjr, vendored in `thirdparty/`
- [caravel](https://github.com/joeymeere/caravel) by Joey Meere, vendored in `thirdparty/`

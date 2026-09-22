# Strategy Backtest Results & Conclusions

A record of the strategies built and backtested in this project, the numbers they
produced, and what we learned. **Everything here is dry-run / backtest research — no
live trading, no money at risk. Backtest profits are not a promise of future profits.**

---

## TL;DR

- Goal evolved to: **trade BTC + ETH only** (biggest = least rug/delist risk), and find a
  sane holding horizon.
- We tested the same idea (RSI / trend / momentum) across horizons. The clear pattern:
  **the less you trade, the better it did. Intraday was the only approach that lost money.**
- Best risk-adjusted result: **`BTCETHCombo`** — a 4h swing gated by the daily 200-SMA
  trend. Highest return-per-unit-risk, lowest drawdown of the BTC+ETH strategies.
- **Out-of-sample reality (see below):** in the unseen 2022–2023 bear, Combo was **flat
  (+0.8%) while the market fell −24%** and it did **not blow up** — but it also made no
  real money. It is a **risk-management tool (rides uptrends, sits out crashes), not an
  all-weather alpha machine.** Do not extrapolate the +79% — that was a bull market.

**Ranking on BTC+ETH, 2024-01-01 → 2026-09-18 (fee 0.075%, incl. in results):**

| Rank | Strategy | Timeframe | Return | Max Drawdown | Profit Factor | Trades | Win% |
|---|---|---|---|---|---|---|---|
| 🥇 | **BTCETHCombo** (recommended) | 4h + daily gate | **+67.9%** | **−26.0%** | 1.56 | 171 | 25% |
| 🥈 | BTCETHSwing | 4h | +78.2% | −32.7% | 1.39 | 248 | 24% |
| 🥉 | BTCETHTrend | 1d | +56.8% | −29.0% | 3.02 | 26 | 35% |
| — | Buy & hold (50/50) | — | ~+54% | ~−50% | — | 0 | — |
| ❌ | BTCETHIntraday | 15m | **−42.1%** | −49.5% | 0.68 | 454 | 66% |

> Drawdown = wallet-balance basis (includes open-position unrealized loss) — the honest number.
> "Market change" over this window was +38–54% depending on resolution; every non-intraday
> strategy beat its own buy-hold benchmark, the intraday one lost badly.

---

## Setup & method

- **Engine:** Freqtrade `2026.6`, installed from PyPI into `.venv` (not vendored). See
  `requirements.txt`. TA-Lib 0.6.8 + scipy installed cleanly on Windows/Py3.13.
- **Data source / geo-block:** `api.binance.com` is HTTP 451 (geo-blocked) here; Bybit/Kraken
  also failed. Fix (in every config): route ccxt's public API to
  `https://data-api.binance.vision/api/v3` (Binance's unauthenticated public-data mirror) +
  `options.fetchMarkets:["spot"]` to skip the geo-blocked futures endpoint. **Data-only — it
  cannot place orders**, so live/dry-run trading still needs a reachable trading endpoint.
- **Fee:** `0.00075` (0.075%, BNB-tier) applied in backtests. Slippage is **not** modeled.
- **Data:** BTC/USDT + ETH/USDT, 5m/15m from 2024-01, 1h/4h/1d from 2021-10 (deep warm-up).
- **Universe rationale:** BTC + ETH only — largest, most liquid, can't delist/go to zero.
  This removes the survivorship-bias problem that inflates alt-coin backtests.

---

## The strategies (files in `user_data/strategies/`)

| File | Horizon | Idea | Verdict |
|---|---|---|---|
| `SampleRSIStrategy.py` | 5m | RSI<30 **AND** close>ema50 (project sample) | **0 trades** — conditions never co-occur; kept as a cautionary example |
| `RSIMeanReversion.py` | 5m | RSI<30 buy / RSI>70 sell (textbook) | Naive baseline: 64% win but **−0.93%** — fees + negative expectancy |
| `NostalgiaForInfinityX8.py` | 5m (alts) | Downloaded community strategy (52k lines) | See note below — not for BTC-only |
| `BTCETHIntraday.py` | 15m | RSI dip + 1h trend filter, quick exits | **−42%** — day-trading loses (fees + noise) |
| `BTCETHTrend.py` | 1d | Hold while price > 200-day SMA, else cash | +57%, lowest turnover, most hands-off |
| `BTCETHSwing.py` | 4h | Momentum: long while EMA50>EMA200 & price>EMA50 | +78%, highest return, biggest swings |
| `BTCETHCombo.py` | 4h + 1d | **Swing timing gated by the daily trend** | **Recommended** — best risk-adjusted |

---

## Key findings

1. **Frequency vs performance (the headline).** Intraday (15m, ~10h holds) → **−42%**.
   Swing (4h, ~8-day holds) → +78%. Daily trend (~40-day holds) → +57%. Combo → +68%.
   Trading more made it worse; the only loser traded the most.
2. **High win rate ≠ profit.** The intraday strategy won 66% of trades and still lost 42%:
   small capped wins vs larger stops = negative expectancy. Win rate is a vanity metric;
   **profit factor** (>1) and expectancy are what matter.
3. **Fees are decisive for frequent trading.** The naive 5m RSI was *positive before fees*
   and *negative after* — ~415 round-trips of 0.075–0.1% ate the thin edge.
4. **Momentum/trend works better at longer horizons** — consistent with the academic
   literature (Faber 2007 on 200-day MA timing; Liu & Tsyvinski 2021 / Liu, Tsyvinski & Wu
   2022 finding crypto time-series momentum at 1–4 week horizons, strongest in large coins).
5. **Combining beats either alone (risk-adjusted).** Using the daily 200-SMA as an on/off
   gate for the 4h swing kept ~90% of the swing's return while cutting drawdown to the
   lowest of all (−26%) and lifting profit factor 1.39 → 1.56.

### Note on NostalgiaForInfinity (NFI)
NFI X8 backtested at **+133% while the alt market fell −59%** over 2024→now (31 pairs, 97%
win rate, 8.5% closed-trade drawdown) — impressive, BUT:
- **Survivorship bias:** the pairlist is coins that *still exist today*; delisted/dead coins
  are excluded, which flatters any backtest over a −59% market.
- **Overfitting:** thousands of hand-tuned conditions fitted to past data.
- **Useless for BTC-only:** on BTC/USDT alone it made **0 trades** — it's an alt-rotation
  strategy, structurally wrong for a BTC+ETH mandate.

---

## Recommended strategy: `BTCETHCombo`

**How it works**
- **Gate (safety):** only trade while the **daily** close is above its **200-day SMA**
  (big-trend up). Bear market → gate shut → sit in cash.
- **Timing (entry/exit):** on the **4h** chart, hold while `EMA50 > EMA200` and price is
  above EMA50; exit when price drops below EMA50 **or** the daily gate closes.

**Behavior**
- Holds **~8 days** on winners, **~1 day** on losers (cuts mistakes fast, lets winners run).
- ~171 trades over 2.7 years ≈ a trade every ~6 days. Check the 4h chart a few times a week —
  **this is not day-trading.**
- **Win rate ~25%** — expect long losing streaks of small stops; the few multi-week winners
  carry it. Requires discipline to keep following signals.
- Max drawdown ~26% — still a real, painful loss; "safer than buy-and-hold," not "safe."

---

## Out-of-sample validation (2022–2023 bear market)

`BTCETHCombo` uses only 4h + daily data, so it can be tested on 2022–2023 — a period that
was **never looked at** while building it, and the worst crypto bear on record (LUNA, FTX,
BTC 69k → 16k). This is the real test of whether the edge is genuine or curve-fit.

| Metric | Out-of-sample 2022–2023 (bear, unseen) | In-sample 2024→now (bull) |
|---|---|---|
| Total return | **+0.82%** (flat) | +78.9% |
| Market change (buy-hold) | **−24.3%** | +62.3% |
| Max drawdown | 27.2% | 26.0% |
| Profit factor | 1.01 (breakeven) | 1.65 |
| Sharpe (daily) | 0.12 | 1.05 |
| Trades | 122 | 171 |

**What this proves (the good):** it did **not blow up**. In a −24% bear it stayed flat and
beat buy-hold by ~25 points, purely by moving to cash — confirming its positive-skew,
"cut-and-run" character (the opposite of an XIV-style short-vol trap). Drawdown was stable
(~27%) across both regimes.

**What this reveals (the sobering):** its big returns are **regime-dependent, not a
standalone edge.** Out-of-sample it made essentially nothing (PF 1.01, Sharpe 0.12) — it
only *protected*. The realistic characterization:

> A **long-biased trend rider with a safety brake.** Bull/trending market → big gains.
> Bear/choppy market → roughly flat, sits in cash, avoids the crash. ~26–27% drawdown
> throughout. It wins long-term by *participating in up-moves and dodging crashes*, not by
> predicting. **Do not extrapolate the +79%.**

## What the backtest does NOT prove

- **Out-of-sample: done (see section above).** Combo survived the 2022–2023 bear (flat,
  no blow-up) but made no money there — returns are regime-dependent. Still **not**
  forward/paper-validated on future data.
- **No slippage modeled** (only fees). Real fills are worse, especially in fast moves.
- **Parameters (EMA50/200, SMA200) are canonical, not hyperopt-tuned** — a plus (less
  overfit) but not optimized.
- Past performance over 2024–2026 says nothing guaranteed about the future.

---

## How to run

```bash
# 1. Install engine (into .venv)
pip install -r requirements.txt

# 2. Download data (uses the data-api.binance.vision workaround already in the configs)
freqtrade download-data --config config-swing.json --pairs BTC/USDT ETH/USDT \
  --timeframe 5m 15m 1h 4h 1d --timerange 20211001-

# 3. Backtest each
freqtrade backtesting --config config-trend.json --strategy BTCETHTrend    --timerange 20240101-
freqtrade backtesting --config config-swing.json --strategy BTCETHSwing    --timerange 20240101-
freqtrade backtesting --config config-swing.json --strategy BTCETHCombo    --timerange 20240101-
```

Configs: `config-trend.json` (1d), `config-swing.json` (4h), `config-day.json` (15m intraday),
`config-nfi.json` (NFI, 40 alt pairs). Real configs are gitignored where they may hold keys.

---

## Next steps

1. **Out-of-sample: DONE** — tested on the unseen 2022–2023 bear (see section above):
   no blow-up, but flat returns → it's a risk-management tool, not standalone alpha.
   Remaining: **forward / paper (dry-run) validation** on future data for a few weeks
   (needs a reachable trading endpoint — the data-only mirror can't place orders).
2. **Light parameter robustness check** — confirm results don't collapse if EMA/SMA lengths
   move a bit (fragile parameters = overfit).
3. Only after sustained dry-run success, and a proper risk review, consider anything live.

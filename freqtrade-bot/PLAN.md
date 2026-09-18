# Project Plan — Crypto Trading Bot (Freqtrade)

## Context

Build a crypto algorithmic trading bot in Python to learn the full pipeline.
Decisions made:

- **Market:** Crypto — easiest access, free data, 24/7, best paper-trading support.
- **Framework:** Freqtrade (open-source) — don't reinvent data/backtesting/execution; write strategy logic only.
- **Stage:** Learning / **paper trading only**. No real money yet.
- **Language:** Python (correct for second-to-minute strategies; true microsecond HFT is out of scope and impossible in pure Python).
- **Repo layout (Option A):** only *my* work (strategy + config) lives in `finance/freqtrade-bot/`; the engine is installed from pip. Keeps the repo small and truly mine.

Reality check: the framework is the easy 20%. A profitable **edge** is the hard 80%,
found through backtesting and iteration. No shipped strategy is reliably profitable.

## Status

- [x] Approach chosen (Option A) and repo scaffolded + pushed to `github.com/ncu571633/finance`
- [x] `requirements.txt` pins `freqtrade==2026.6`
- [x] `config.example.json` (template, no secrets) + `config.json` (real, gitignored) created
- [x] Starter strategy written: `user_data/strategies/SampleRSIStrategy.py`
- [x] Engine installed into `.venv` (Python 3.13). TA-Lib 0.6.8 installed cleanly
      (prebuilt Windows wheels now exist — the old TA-Lib build pain is gone).
- [x] `scipy==1.18.1` installed and pinned in `requirements.txt` (was the BLOCKED item).
- [x] **Data geo-block solved.** api.binance.com / bybit / kraken-klines all failed
      here (451 / 403 / no-klines). Fix in `config.json`: route ccxt's public API to
      `https://data-api.binance.vision/api/v3` (Binance's unauthenticated public-data
      mirror) + `options.fetchMarkets:["spot"]` to skip the geo-blocked futures endpoint.
      NOTE: this endpoint serves DATA ONLY — it cannot place orders, so dry-run/live
      trading will still need a reachable trading endpoint (see open question below).
- [x] Downloaded 180 days of 5m data for BTC/USDT + ETH/USDT (52,083 candles each).
- [x] Backtested. `SampleRSIStrategy` → **0 trades** (entry "RSI<30 AND close>ema50"
      is self-contradictory on 5m: verified RSI<30 and close>ema50 never co-occur).
      Added `RSIMeanReversion.py` (textbook RSI<30 buy / RSI>70 sell) → **415 trades,
      64% win rate, but −1.44% total** (high win rate, negative expectancy — small ROI
      wins wiped out by −5% stops + signal-exit losses; profit factor 0.78, Sharpe −3.6).
      Pipeline confirmed working; no edge yet (as expected).
- [ ] (Open) Trading endpoint for dry-run: data.binance.vision is data-only. Options:
      run dry-run against a reachable exchange (needs a trading host that isn't geo-blocked),
      or keep iterating in backtest for now.
- [ ] Iterate toward an actual edge (trend filter, ATR stops, hyperopt, higher timeframe).
- [ ] (Later) dry-run paper trading once a trading endpoint is sorted.

## Next steps (run from this folder)

```bash
# 0. Fix the missing dependency
pip install scipy
#    then pin it: add "scipy" to requirements.txt

# 1. Download history (public data — no keys needed)
freqtrade download-data --config config.json \
  --pairs BTC/USDT ETH/USDT --timeframe 5m --days 180

# 2. Backtest
freqtrade backtesting --config config.json \
  --strategy SampleRSIStrategy --timeframe 5m --timerange 20240101-

# 3. Read results: profit %, win rate, max drawdown.
#    Losses are expected — the goal is to confirm the pipeline runs.
```

## Known gotchas

- **Native Windows + Python 3.13:** dependency install can be flaky (this is why
  `scipy` was missing, and why TA-Lib sometimes fails). A venv on Python 3.12 (via
  `uv` or WSL) is the tidier long-term setup.
- **Binance geo-restrictions:** if `download-data` returns HTTP 451, switch the
  exchange in `config.json` (e.g. to `kraken`) or use a VPN-free public data source.
- **Secrets:** never commit `config.json` (gitignored). Only `config.example.json`
  with placeholders is tracked.

## Guardrails

- Keep `dry_run: true` the whole time; do not grant API withdrawal permissions.
- Treat backtest profits skeptically (overfitting, lookahead bias, unrealistic fills).
- No real money until profitable in dry-run over weeks — not just in backtest.

## Out of scope (later phases)

- Live trading with real capital (needs full risk-management review first)
- Custom-built backtester from scratch (learn Freqtrade internals first)
- Polymarket / prediction-market arbitrage (different mechanics, harder access)

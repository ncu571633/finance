# freqtrade-bot

My crypto trading bot — **only my own work is stored here** (strategies + config).
The Freqtrade engine is installed from PyPI (see `requirements.txt`), not vendored
into this repo. Stage: **paper trading (dry-run) only**.

## What's in here (and what isn't)

| In this repo (my code) | NOT in this repo (comes from pip) |
|---|---|
| `user_data/strategies/*.py` — my strategies | The Freqtrade engine: login, pricing, order execution, backtesting |
| `config.example.json` — config template (no secrets) | Historical data, logs, DB (regenerated locally, gitignored) |
| `requirements.txt` — pins `freqtrade==2026.6` | |

## Setup

```bash
# 1. Create a virtual environment (use a Freqtrade-supported Python: 3.10–3.13)
python -m venv .venv
# Windows:  .venv\Scripts\activate      Linux/Mac:  source .venv/bin/activate

# 2. Install the engine
pip install -r requirements.txt

# 3. Create your REAL config from the template (this file is gitignored)
cp config.example.json config.json
#   For dry-run you can leave the placeholder keys; add real read-only keys later.
```

> ⚠️ **Never commit `config.json`** — it holds API keys and is gitignored. Only
> `config.example.json` (with placeholders) is tracked.

## Workflow

```bash
# Download historical data for backtesting
freqtrade download-data --config config.json \
  --pairs BTC/USDT ETH/USDT --timeframe 5m 1h --days 180

# Backtest the strategy
freqtrade backtesting --config config.json \
  --strategy SampleRSIStrategy --timeframe 5m --timerange 20240101-

# Paper trade with live prices (dry_run: true in config → simulated orders)
freqtrade trade --config config.json --strategy SampleRSIStrategy
```

## The strategy is the part I own

A strategy is three methods in `user_data/strategies/SampleRSIStrategy.py`:

| Method | Purpose |
|---|---|
| `populate_indicators()` | Compute indicators (RSI, EMA, …) with pandas + TA-Lib |
| `populate_entry_trend()` | Rules for **when to buy** (`enter_long`) |
| `populate_exit_trend()` | Rules for **when to sell** (`exit_long`) |

Everything else (auth, pricing, orders, position management) is handled by the
engine. Edit these methods to encode a real edge, then backtest.

## Guardrails (learning stage)
- Keep `dry_run: true` in `config.json`.
- Do not grant the API key withdrawal permissions.
- Treat backtest profits skeptically (overfitting, lookahead bias). Fees + slippage
  are applied automatically — verify in the backtest output.
- No real money until a strategy is profitable in dry-run over weeks, not just backtest.

## Updating the engine
```bash
pip install --upgrade freqtrade   # then update the pin in requirements.txt
```

## References
- Install: https://www.freqtrade.io/en/stable/installation/
- Strategy customization: https://www.freqtrade.io/en/stable/strategy-customization/

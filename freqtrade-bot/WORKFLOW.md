# Freqtrade Workflow

How Freqtrade works — both the **runtime engine loop** (what the bot does when trading)
and the **development workflow** (what you do to build and test a strategy).

---

## 1. Development workflow (what you run)

```
 install engine        pip install -r requirements.txt   (engine → site-packages, NOT this folder)
        │
 create config         cp config.example.json config.json   (dry_run: true; gitignored)
        │
 download history      freqtrade download-data --config config.json \
        │                --pairs BTC/USDT ETH/USDT --timeframe 5m --days 180
        │              → user_data/data/binance/   (gitignored)
        │
 write strategy        edit user_data/strategies/SampleRSIStrategy.py
        │                (the 3 populate_* methods — the part you own)
        │
 backtest              freqtrade backtesting --config config.json \
        │                --strategy SampleRSIStrategy --timeframe 5m --timerange 20240101-
        │
 (optional) optimize   freqtrade hyperopt ...     (tune params; beware overfitting)
        │
 paper trade           freqtrade trade --config config.json --strategy SampleRSIStrategy
        │                (live prices, SIMULATED orders because dry_run: true)
        │
 (later, not now)      real money — only after sustained dry-run success
```

Each command is handled under `freqtrade/commands/` in the engine.

---

## 2. Runtime engine loop (what the bot does when trading)

When you run `freqtrade trade`, the engine repeats this loop every few seconds
(`process_throttle_secs` in config):

```
        ┌─────────────────────────────────────────────────────────┐
        │  worker.py: _worker() → _throttle() → _process_running() │
        │                         │                                 │
        │                         ▼                                 │
        │        freqtradebot.py: process()  (one iteration)        │
        └─────────────────────────────────────────────────────────┘
                                  │
   ┌──────────────────────────────┼───────────────────────────────┐
   ▼                              ▼                                ▼
1. LOGIN / CONNECT          2. GET MARKET DATA            3. COMPUTE SIGNALS
 exchange/exchange.py        exchange/exchange.py           YOUR strategy:
 - reads API key/secret      - fetch_ticker(pair)           - populate_indicators()
   from config.json          - get_tickers()                - populate_entry_trend()
 - hands them to ccxt        - OHLCV candles                - populate_exit_trend()
 - _api_reload_markets()     - exchange_ws.py (WebSocket)
                                  │
                                  ▼
4. DECIDE & EXECUTE TRADES                     5. PERSIST STATE
 freqtradebot.py:                               persistence/  (SQLite DB)
 - enter_positions() → create_trade()           - open/closed trades
     → execute_entry()                          wallets.py
 - exit_positions() → handle_trade()            - balances
     → execute_trade_exit()                    rpc/  (Telegram / REST notify)
 - orders via exchange.create_order()
   (dry-run: create_dry_run_order() → simulated, no real money)
```

The loop runs continuously until stopped. In **dry-run** mode step 4 places
*simulated* orders against *real* live prices — no money moves.

---

## 3. Where each capability lives (engine internals — read-only)

| Capability | File / function |
|---|---|
| **Login / exchange auth** | `freqtrade/exchange/exchange.py` — keys from `config.json` → `ccxt`; `_api_reload_markets()`, `get_balances()`. ~25 exchange adapters (`binance.py`, `kraken.py`, …). |
| **Get current price** | `exchange.py`: `fetch_ticker()`, `get_tickers()`, `get_rate()`; live feeds in `exchange_ws.py` |
| **Historical data** | `freqtrade/data/` — download + local storage (Parquet/JSON) |
| **Trade execution** | `freqtrade/freqtradebot.py`: `process()` → `enter_positions()`/`exit_positions()` → `execute_entry()`/`execute_trade_exit()` |
| **Main loop / lifecycle** | `freqtrade/worker.py`: `_worker()`, `_throttle()`, `_process_running()` |
| **Persistence (trades DB)** | `freqtrade/persistence/` (SQLite) |
| **Balances / wallet** | `freqtrade/wallets.py` |
| **Backtesting & hyperopt** | `freqtrade/optimize/` |
| **Notifications / control** | `freqtrade/rpc/` (Telegram, REST API, webhooks) |
| **Pairlists & protections** | `freqtrade/plugins/` |
| **CLI commands** | `freqtrade/commands/` |

---

## 4. The one part you write: the Strategy

`user_data/strategies/SampleRSIStrategy.py` — a class with three methods:

| Method | Purpose |
|---|---|
| `populate_indicators(df, meta)` | Compute indicators (RSI, EMA, …) with pandas + TA-Lib |
| `populate_entry_trend(df, meta)` | Rules for **when to buy** (`enter_long`) |
| `populate_exit_trend(df, meta)` | Rules for **when to sell** (`exit_long`) |

Plus class knobs: `timeframe`, `minimal_roi`, `stoploss`, `startup_candle_count`.
Everything else (auth, pricing, orders, persistence) is the engine — you don't touch it.

---

## 5. Golden rule

The engine is the easy 20%. A profitable **edge** in the strategy is the hard 80%.
Backtest and paper-trade honestly (fees + slippage included); no shipped strategy is
reliably profitable out of the box.

# stock-backtrader

US-stock version of the trend-filter strategy from `../freqtrade-bot`, using
**backtrader + yfinance** (the crypto work used Freqtrade, which can't trade stocks).

Rule (same as `BTCETHTrend`): **hold while price > N-day SMA, else cash.** Faber (2007)'s
classic 200-day timing model — equities are its original home. **Backtest only, not advice.**

## Setup
```bash
python -m venv .venv
.venv\Scripts\activate          # Windows (Linux/Mac: source .venv/bin/activate)
pip install -r requirements.txt
```

## Run
```bash
# Daily 200-day SMA filter
python stock_trend.py --ticker SPY --start 2005-01-01 --sma 200

# Faber's original: month-end bars + 10-month SMA (recommended for stocks)
python stock_trend.py --ticker SPY --start 2005-01-01 --monthly
```
Data comes free from Yahoo Finance (no API key). Prints strategy vs buy-and-hold
(return + max drawdown), trades, and win rate.

## Findings (2005–2026, fee 0.05%)

Daily SMA vs Faber's monthly (10-month) rebalance:

| Ticker / method | Return | Buy & hold | Max DD | Trades | Win% |
|---|---|---|---|---|---|
| SPY daily SMA200 | +372% | +855% | −23.5% | 62 | 31% |
| **SPY monthly 10mo** | +385% | +855% | −22.3% | **16** | **63%** |
| QQQ daily SMA200 | +867% | +2108% | −25.7% | 62 | 24% |
| **QQQ monthly 10mo** | +875% | +2108% | −24.7% | **18** | **61%** |

**Monthly is the better implementation for stocks:** same drawdown protection, but ~4×
fewer trades (less whipsaw, fewer taxes/fees), and a ~60% win rate (far easier to follow).
It's Faber's canonical, decades-validated method — not a cherry-picked parameter.

**Key insight — opposite verdict vs crypto.** On BTC+ETH the trend filter *beat*
buy-and-hold; on US index ETFs it *underperforms* buy-and-hold badly but roughly **halves
the drawdown**. US equities grind up and V-recover fast, so the filter costs return. Index
buy-and-hold is very hard to beat.

**Caution — parameter sensitivity:** daily QQQ returned +412% / +1039% / +867% at SMA
100 / 150 / 200 — a 2.5× swing from a small parameter change = overfitting risk. Trust the
*stable* result (drawdown roughly halved across all settings), not any single peak number.

**So on stocks this strategy is a drawdown-reduction (risk) tool, not an alpha tool.**
Use it only if you'd otherwise not stomach a −55% crash; if you can hold, plain
index buy-and-hold / DCA won here.

## Files
| File | Purpose |
|---|---|
| `stock_trend.py` | The backtest (yfinance data + backtrader SMA-trend strategy) |
| `requirements.txt` | backtrader, yfinance, pandas |
| `.venv/` | Python env (create locally; not committed) |

## Next ideas
- Try other SMA lengths (100/150/200), or a dual-MA, or monthly rebalance (Faber's original).
- Add position sizing / a small allocation to trend as "crash insurance" alongside buy-hold.
- Paper-trade via Alpaca or Interactive Brokers before anything live.

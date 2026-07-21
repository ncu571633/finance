# Freqtrade Strategy Guide

Annotated walkthrough of Freqtrade's official strategy template
(`freqtrade/templates/sample_strategy.py`, class `SampleStrategy`, INTERFACE_VERSION 3)
— what every part does and how to write your own.

A strategy is a single Python class that subclasses `IStrategy`. You provide
**indicators** and **buy/sell rules**; the engine handles data, orders, and money.

---

## 1. Anatomy at a glance

```
class SampleStrategy(IStrategy):
    # (a) class attributes ......... config knobs (ROI, stoploss, timeframe, …)
    # (b) hyperopt parameters ...... values hyperopt can tune (buy_rsi, sell_rsi, …)
    # (c) plot_config .............. what to draw in `freqtrade plot-dataframe`
    def informative_pairs(self):        # (optional) extra pairs/timeframes to fetch
    def populate_indicators(df, meta):  # MANDATORY — compute indicators
    def populate_entry_trend(df, meta): # MANDATORY — when to buy (enter_long/short)
    def populate_exit_trend(df, meta):  # MANDATORY — when to sell (exit_long/short)
```

Three methods are **mandatory**: `populate_indicators`, `populate_entry_trend`,
`populate_exit_trend`. Everything else is optional but recommended.

---

## 2. Imports

```python
import talib.abstract as ta          # TA-Lib: RSI, MACD, ADX, SAR, TEMA, …
from technical import qtpylib        # helpers: bollinger_bands, crossed_above, typical_price
from freqtrade.strategy import (
    IStrategy, Trade, Order, PairLocks, informative,
    BooleanParameter, CategoricalParameter, DecimalParameter, IntParameter, RealParameter,
    timeframe_to_minutes, merge_informative_pair, stoploss_from_open, ...
)
```

`ta` and `qtpylib` are the two indicator libraries you'll use most. The
`*Parameter` classes are only needed if you want hyperopt to tune values.

---

## 3. Class attributes (config knobs)

| Attribute | Sample value | Meaning |
|---|---|---|
| `INTERFACE_VERSION` | `3` | Strategy API version. Keep at 3. |
| `can_short` | `False` | Allow short trades (needs futures + margin). Spot = False. |
| `minimal_roi` | `{"60":0.01,"30":0.02,"0":0.04}` | Take-profit ladder: **minutes held → min ROI to exit**. Here: +4% immediately, +2% after 30 min, +1% after 60 min. |
| `stoploss` | `-0.10` | Hard stop: exit at −10%. **Required.** |
| `trailing_stop` | `False` | Enable trailing stop (+ `trailing_stop_positive*` knobs). |
| `timeframe` | `"5m"` | Candle size the strategy runs on. |
| `process_only_new_candles` | `True` | Recompute indicators only on a new candle (faster). |
| `use_exit_signal` | `True` | Honor `populate_exit_trend` signals (vs. ROI/stoploss only). |
| `exit_profit_only` | `False` | If True, only act on exit signals when in profit. |
| `ignore_roi_if_entry_signal` | `False` | If True, don't ROI-exit while an entry signal persists. |
| `startup_candle_count` | `200` | Warm-up candles before signals are valid — **must be ≥ your longest indicator period** (e.g. 200-EMA → ≥200), or early signals use incomplete data. |
| `order_types` | limit entry/exit, market stop | How orders are placed on the exchange. |
| `order_time_in_force` | `{"entry":"GTC","exit":"GTC"}` | Order lifetime policy. |
| `plot_config` | dict | Which indicators `freqtrade plot-dataframe` draws. |

> Anything set in `config.json` **overrides** the class attribute (e.g. `minimal_roi`,
> `stoploss`, `timeframe`).

---

## 4. Hyperopt parameters

```python
buy_rsi  = IntParameter(low=1,  high=50,  default=30, space="buy",  optimize=True, load=True)
sell_rsi = IntParameter(low=50, high=100, default=70, space="sell", optimize=True, load=True)
```

These behave like normal values (`self.buy_rsi.value` → `30`) but `freqtrade hyperopt`
can search the `low..high` range to find better numbers. Parameter types:
`IntParameter`, `DecimalParameter`, `RealParameter`, `CategoricalParameter`,
`BooleanParameter`. `space` groups them into `buy` / `sell` / `exit` / `roi` / `stoploss`.
Skip these entirely if you're not optimizing yet — just use plain numbers.

---

## 5. `populate_indicators(dataframe, metadata)` — MANDATORY

Runs **once per candle over the whole DataFrame** (vectorized — you operate on entire
columns, not row-by-row). Add a column per indicator:

```python
dataframe["rsi"]  = ta.RSI(dataframe)                 # TA-Lib
dataframe["sar"]  = ta.SAR(dataframe)
dataframe["tema"] = ta.TEMA(dataframe, timeperiod=9)

macd = ta.MACD(dataframe)                              # multi-output indicator
dataframe["macd"]       = macd["macd"]
dataframe["macdsignal"] = macd["macdsignal"]

bollinger = qtpylib.bollinger_bands(qtpylib.typical_price(dataframe), window=20, stds=2)
dataframe["bb_lowerband"]  = bollinger["lower"]
dataframe["bb_middleband"] = bollinger["mid"]
dataframe["bb_upperband"]  = bollinger["upper"]

return dataframe   # ALWAYS return the dataframe
```

The `dataframe` already has `open/high/low/close/volume` columns. The sample computes
ADX, RSI, Stochastic Fast, MACD, MFI, Bollinger Bands, Parabolic SAR, TEMA, and a
Hilbert sine — most other indicators are shown commented-out as a menu.

⚠️ **Performance:** each indicator costs memory/CPU. Compute only what your rules use.
⚠️ **Lookahead bias:** never use future data. Vectorized indicators are safe;
`.shift(1)` looks *backward* (previous candle) which is fine.

---

## 6. `populate_entry_trend(dataframe, metadata)` — MANDATORY

Set the column **`enter_long`** (and/or `enter_short`) to `1` on rows where your buy
condition holds. This is a **vectorized boolean mask**, not a loop:

```python
dataframe.loc[
    (
        (qtpylib.crossed_above(dataframe["rsi"], self.buy_rsi.value))  # trigger: RSI crosses up through 30
        & (dataframe["tema"] <= dataframe["bb_middleband"])            # guard: TEMA below BB middle
        & (dataframe["tema"] > dataframe["tema"].shift(1))             # guard: TEMA rising vs prev candle
        & (dataframe["volume"] > 0)                                    # guard: candle has volume
    ),
    "enter_long",
] = 1
return dataframe
```

Pattern: **one trigger + several guards**, combined with `&` (and), `|` (or). Wrap each
condition in parentheses. `crossed_above(a, b)` is true only on the candle where `a`
crosses from below to above `b` (an event, not a level). `.shift(1)` = previous candle's
value. `enter_short` works the same for shorting (needs `can_short = True`).

---

## 7. `populate_exit_trend(dataframe, metadata)` — MANDATORY

Set **`exit_long`** (and/or `exit_short`) to `1` where you want to close:

```python
dataframe.loc[
    (
        (qtpylib.crossed_above(dataframe["rsi"], self.sell_rsi.value))  # RSI crosses up through 70
        & (dataframe["tema"] > dataframe["bb_middleband"])              # guard: TEMA above BB middle
        & (dataframe["tema"] < dataframe["tema"].shift(1))              # guard: TEMA falling
        & (dataframe["volume"] > 0)
    ),
    "exit_long",
] = 1
return dataframe
```

Note: exits also happen automatically via `minimal_roi` (take-profit) and `stoploss` —
`populate_exit_trend` is the *signal-based* exit on top of those. Set
`use_exit_signal = False` to rely on ROI/stoploss only.

---

## 8. The sample strategy in plain English

- **Buy (long)** when RSI crosses above 30 *and* TEMA is below the Bollinger midline but
  turning up *and* there's volume → "oversold and starting to recover."
- **Sell (long)** when RSI crosses above 70 *and* TEMA is above the midline and turning
  down → "overbought and rolling over." Also auto-exits at +4%/+2%/+1% ROI or −10% stop.
- Short rules mirror these (only if `can_short = True` on futures).

This is a generic demo of *mechanics* — it is **not** a profitable strategy. Treat it
as a skeleton to replace with your own edge.

---

## 9. Optional hooks (advanced — add only when needed)

Defined on `IStrategy`, override to customize behavior:

| Method | Use |
|---|---|
| `informative_pairs()` | Declare extra pairs/timeframes to pull (e.g. 1h trend filter for a 5m strategy) |
| `custom_stoploss(...)` | Dynamic per-trade stoploss |
| `custom_entry_price()` / `custom_exit_price()` | Override limit-order price |
| `confirm_trade_entry(...)` / `confirm_trade_exit(...)` | Final veto before an order is placed (e.g. check live order book) |
| `custom_stake_amount(...)` | Per-trade position sizing |
| `leverage(...)` | Set leverage (futures) |
| `@informative(timeframe)` decorator | Cleaner way to add higher-timeframe indicators |

Ignore all of these until your entry/exit logic is solid.

---

## 10. How to write your own

1. Copy the template or edit `user_data/strategies/SampleRSIStrategy.py`.
2. Rename the class (and pass `--strategy <ClassName>` on the CLI — the class name, not
   the file name, is what's referenced).
3. In `populate_indicators`, compute only the indicators your idea needs.
4. In `populate_entry_trend` / `populate_exit_trend`, encode the rules as boolean masks.
5. Set `timeframe`, `stoploss`, `minimal_roi`, and `startup_candle_count`.
6. Backtest, read results, iterate:
   ```bash
   freqtrade backtesting --config config.json --strategy SampleRSIStrategy \
     --timeframe 5m --timerange 20240101-
   ```

## Checklist / gotchas

- [ ] All three `populate_*` methods `return dataframe`.
- [ ] `startup_candle_count` ≥ your longest indicator period.
- [ ] Only compute indicators you actually use (performance).
- [ ] Wrap each condition in `()`; combine with `&` / `|`.
- [ ] `crossed_above/below` = event (one candle); bare comparison = level (every candle).
- [ ] `.shift(1)` looks backward (safe); never reference future rows (lookahead bias).
- [ ] Class name passed to `--strategy`, not the filename.
- [ ] Backtest profits ≠ real profits — always include fees/slippage and paper-trade before trusting.

# BTCETHIntraday — an intraday (same-day) RSI-dip strategy for BTC + ETH.
#
# Day-trading is the HARDEST mode: many trades => fees dominate, and short horizons
# are close to noise. So this is a "least-bad" intraday design, not a money printer:
#
#   Trend filter : only go long while the 1h chart is in an uptrend (close > 1h EMA200).
#                  This stops us catching falling knives in a downtrend.
#   Entry        : on the 15m chart, RSI(14) is oversold (< 30)  -> buy the dip.
#   Exit         : quick — RSI overbought (> 70), OR the ROI ladder / stop-loss below.
#                  Holds are short (hours), i.e. genuinely intraday.
#
# Long-only, spot. Backtest honestly (fees + slippage) before trusting anything.

from pandas import DataFrame
import talib.abstract as ta

from freqtrade.strategy import IStrategy, merge_informative_pair


class BTCETHIntraday(IStrategy):
    INTERFACE_VERSION = 3

    timeframe = "15m"
    can_short = False

    # Quick intraday take-profit ladder (minutes held -> min ROI to exit).
    minimal_roi = {
        "0": 0.025,   # +2.5% any time
        "120": 0.015, # +1.5% after 2h
        "360": 0.005, # +0.5% after 6h
        "720": 0.0,   # break-even after 12h -> keeps trades same-day-ish
    }

    stoploss = -0.03          # -3% hard stop
    trailing_stop = False

    process_only_new_candles = True
    use_exit_signal = True
    exit_profit_only = False

    # 200-period EMA on the 1h informative => need ~200*4 = 800 base (15m) candles warm-up.
    startup_candle_count = 800

    def informative_pairs(self):
        pairs = self.dp.current_whitelist()
        return [(pair, "1h") for pair in pairs]

    def populate_indicators(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        dataframe["rsi"] = ta.RSI(dataframe, timeperiod=14)

        # 1h trend filter, merged onto the 15m frame (columns get a _1h suffix).
        inf = self.dp.get_pair_dataframe(pair=metadata["pair"], timeframe="1h")
        inf["ema200"] = ta.EMA(inf, timeperiod=200)
        dataframe = merge_informative_pair(dataframe, inf, self.timeframe, "1h", ffill=True)

        dataframe["uptrend_1h"] = dataframe["close_1h"] > dataframe["ema200_1h"]
        return dataframe

    def populate_entry_trend(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        dataframe.loc[
            (
                (dataframe["rsi"] < 30)          # 15m oversold dip
                & (dataframe["uptrend_1h"])      # only in a 1h uptrend
                & (dataframe["volume"] > 0)
            ),
            "enter_long",
        ] = 1
        return dataframe

    def populate_exit_trend(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        dataframe.loc[
            (
                (dataframe["rsi"] > 70)          # 15m overbought -> take profit
                & (dataframe["volume"] > 0)
            ),
            "exit_long",
        ] = 1
        return dataframe

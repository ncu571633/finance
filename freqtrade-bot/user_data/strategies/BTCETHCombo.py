# BTCETHCombo — combines the two strategies instead of averaging them.
#
#   Big-picture safety gate  = the DAILY trend  (price > 200-day SMA)  -> from BTCETHTrend
#   Entry/exit timing        = the 4h SWING momentum (EMA50 / EMA200)  -> from BTCETHSwing
#
# Rule: only take 4h swing trades while the DAILY chart is in an uptrend. When the daily
# 200-SMA regime turns bearish, the gate closes: no new entries, and any open trade is
# exited. Goal = keep the swing's upside but cut its drawdown by sitting out bear markets.
# Long-only, spot.

from pandas import DataFrame
import talib.abstract as ta

from freqtrade.strategy import IStrategy, merge_informative_pair


class BTCETHCombo(IStrategy):
    INTERFACE_VERSION = 3

    timeframe = "4h"
    can_short = False

    minimal_roi = {"0": 100.0}   # off — let the trend/swing logic decide
    stoploss = -0.10             # loose safety net
    trailing_stop = False

    process_only_new_candles = True
    use_exit_signal = True
    exit_profit_only = False

    # Daily 200-SMA needs 200 daily candles => 200*6 = 1200 base (4h) candles warm-up.
    startup_candle_count = 1200

    def informative_pairs(self):
        pairs = self.dp.current_whitelist()
        return [(pair, "1d") for pair in pairs]

    def populate_indicators(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        # 4h swing indicators
        dataframe["ema_fast"] = ta.EMA(dataframe, timeperiod=50)
        dataframe["ema_slow"] = ta.EMA(dataframe, timeperiod=200)

        # Daily regime gate
        inf = self.dp.get_pair_dataframe(pair=metadata["pair"], timeframe="1d")
        inf["sma200"] = ta.SMA(inf, timeperiod=200)
        dataframe = merge_informative_pair(dataframe, inf, self.timeframe, "1d", ffill=True)
        dataframe["regime_bull"] = dataframe["close_1d"] > dataframe["sma200_1d"]
        return dataframe

    def populate_entry_trend(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        dataframe.loc[
            (
                (dataframe["regime_bull"])                        # daily gate OPEN
                & (dataframe["ema_fast"] > dataframe["ema_slow"]) # 4h uptrend
                & (dataframe["close"] > dataframe["ema_fast"])    # above short trend
                & (dataframe["volume"] > 0)
            ),
            "enter_long",
        ] = 1
        return dataframe

    def populate_exit_trend(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        dataframe.loc[
            (
                (
                    (dataframe["close"] < dataframe["ema_fast"])  # swing momentum faded
                    | (~dataframe["regime_bull"])                 # OR daily gate CLOSED
                )
                & (dataframe["volume"] > 0)
            ),
            "exit_long",
        ] = 1
        return dataframe

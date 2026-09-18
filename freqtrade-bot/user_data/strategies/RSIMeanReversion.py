# RSIMeanReversion — a textbook RSI strategy that ACTUALLY trades.
#
# Why this exists: SampleRSIStrategy's entry ("RSI<30 AND close>ema50") never
# fires on 5m data — when RSI(14) drops below 30, price is essentially always
# already below the 50-EMA, so the two conditions are mutually exclusive (0 trades).
#
# This version is the classic mean-reversion setup so we can see real backtest
# numbers. It is STILL not a proven money-maker — pure RSI reversion on crypto
# typically bleeds to fees. Treat the output as a baseline to beat, not a signal.

from pandas import DataFrame
import talib.abstract as ta

from freqtrade.strategy import IStrategy


class RSIMeanReversion(IStrategy):
    """
    Buy when RSI is oversold (crosses below 30). Sell when RSI is overbought
    (crosses above 70) — plus the ROI ladder and stop-loss below.
    Long-only, spot.
    """

    INTERFACE_VERSION = 3

    timeframe = "5m"
    can_short = False

    # Take-profit ladder: minutes held -> min ROI to exit.
    minimal_roi = {
        "0": 0.03,
        "30": 0.02,
        "60": 0.01,
    }

    stoploss = -0.05
    trailing_stop = False

    startup_candle_count = 30

    def populate_indicators(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        dataframe["rsi"] = ta.RSI(dataframe, timeperiod=14)
        return dataframe

    def populate_entry_trend(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        dataframe.loc[
            (
                (dataframe["rsi"] < 30)        # oversold
                & (dataframe["volume"] > 0)    # tradable candle
            ),
            "enter_long",
        ] = 1
        return dataframe

    def populate_exit_trend(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        dataframe.loc[
            (
                (dataframe["rsi"] > 70)        # overbought
                & (dataframe["volume"] > 0)
            ),
            "exit_long",
        ] = 1
        return dataframe

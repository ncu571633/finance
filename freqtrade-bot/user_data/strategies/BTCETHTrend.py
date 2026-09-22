# BTCETHTrend — a simple daily trend filter for BTC + ETH only.
#
# Idea: the goal for a BTC/ETH-only trader is NOT to out-trade the market, it's to
# capture most of the upside while cutting the brutal 50%+ drawdowns of buy-and-hold.
# Classic way to do that: stay long only while price is above its 200-day SMA, and
# move to cash when it drops below. This sidesteps the worst of bear markets.
#
# Long-only, spot. ROI and stop-loss are effectively disabled so the 200-SMA cross is
# the ONLY thing that opens/closes a position — a pure trend filter, easy to reason about.

from pandas import DataFrame
import talib.abstract as ta

from freqtrade.strategy import IStrategy


class BTCETHTrend(IStrategy):
    INTERFACE_VERSION = 3

    timeframe = "1d"
    can_short = False

    # Disable ROI take-profit and stop-loss: the trend filter decides everything.
    minimal_roi = {"0": 100.0}   # 10000% — never triggers
    stoploss = -0.99             # -99% — effectively off
    trailing_stop = False

    process_only_new_candles = True
    use_exit_signal = True
    exit_profit_only = False

    # Need 200 daily candles of warm-up before the SMA is valid.
    startup_candle_count = 200

    def populate_indicators(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        dataframe["sma200"] = ta.SMA(dataframe, timeperiod=200)
        return dataframe

    def populate_entry_trend(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        dataframe.loc[
            (
                (dataframe["close"] > dataframe["sma200"])  # uptrend
                & (dataframe["volume"] > 0)
            ),
            "enter_long",
        ] = 1
        return dataframe

    def populate_exit_trend(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        dataframe.loc[
            (dataframe["close"] < dataframe["sma200"]),  # trend broke -> go to cash
            "exit_long",
        ] = 1
        return dataframe

# BTCETHSwing — a multi-day "swing" momentum strategy for BTC + ETH (4h timeframe).
#
# Motivation: academic work on crypto (Liu & Tsyvinski 2021; Liu, Tsyvinski & Wu 2022)
# finds a robust TIME-SERIES MOMENTUM effect at ~1-4 week horizons, and that it works
# best in LARGE coins (BTC/ETH). That is neither intraday (noise, fees) nor months-long
# (the daily 200-SMA). This targets that middle "hold for days to ~2 weeks" band.
#
# Logic on the 4h chart:
#   Regime  : only trade long while EMA50 > EMA200  (an uptrend is in force).
#   Hold    : stay long while price is above the EMA50 (the short trend line).
#   Exit    : price closes back below EMA50 (momentum faded) — plus a safety stop.
# Long-only, spot.

from pandas import DataFrame
import talib.abstract as ta

from freqtrade.strategy import IStrategy


class BTCETHSwing(IStrategy):
    INTERFACE_VERSION = 3

    timeframe = "4h"
    can_short = False

    # Let the trend decide the exit; keep only a loose safety stop. No ROI cap so
    # winners can run for the full multi-day swing.
    minimal_roi = {"0": 100.0}   # effectively off
    stoploss = -0.10             # -10% safety net
    trailing_stop = False

    process_only_new_candles = True
    use_exit_signal = True
    exit_profit_only = False

    # EMA200 on 4h needs 200 candles of warm-up.
    startup_candle_count = 200

    def populate_indicators(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        dataframe["ema_fast"] = ta.EMA(dataframe, timeperiod=50)   # ~8 days on 4h
        dataframe["ema_slow"] = ta.EMA(dataframe, timeperiod=200)  # ~33 days on 4h
        return dataframe

    def populate_entry_trend(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        dataframe.loc[
            (
                (dataframe["ema_fast"] > dataframe["ema_slow"])   # uptrend regime
                & (dataframe["close"] > dataframe["ema_fast"])    # above the short trend
                & (dataframe["volume"] > 0)
            ),
            "enter_long",
        ] = 1
        return dataframe

    def populate_exit_trend(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        dataframe.loc[
            (
                (dataframe["close"] < dataframe["ema_fast"])      # momentum faded -> exit
                & (dataframe["volume"] > 0)
            ),
            "exit_long",
        ] = 1
        return dataframe

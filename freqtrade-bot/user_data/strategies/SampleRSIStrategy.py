# SampleRSIStrategy — a minimal, readable starter strategy.
#
# This is the ONE part of the project you own. The Freqtrade engine (login,
# pricing, order execution, backtesting) is installed from pip — see requirements.txt.
#
# Strategy logic below is a generic illustration, NOT a proven money-maker.
# Edit the three populate_* methods to encode your own edge, then backtest.

from pandas import DataFrame
import talib.abstract as ta

from freqtrade.strategy import IStrategy


class SampleRSIStrategy(IStrategy):
    """
    Simple mean-reversion-ish example:
      - Buy when RSI is oversold (< 30) AND price is above the 50-EMA (uptrend filter).
      - Sell when RSI is overbought (> 70).
    """

    INTERFACE_VERSION = 3

    # Candle size the strategy runs on.
    timeframe = "5m"

    # Long-only for simplicity (spot). Set True + configure for shorting/futures.
    can_short = False

    # Take-profit ladder: minutes-held -> minimum ROI to exit.
    minimal_roi = {
        "0": 0.03,   # +3% any time
        "30": 0.02,  # +2% after 30 min
        "60": 0.01,  # +1% after 60 min
    }

    # Hard stop-loss: exit if a trade drops 5%.
    stoploss = -0.05

    trailing_stop = False

    # Enough history for the 50-period EMA to be valid before signals start.
    startup_candle_count = 50

    def populate_indicators(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        """Compute indicators once per candle for the whole dataframe."""
        dataframe["rsi"] = ta.RSI(dataframe, timeperiod=14)
        dataframe["ema50"] = ta.EMA(dataframe, timeperiod=50)
        return dataframe

    def populate_entry_trend(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        """Set enter_long = 1 on rows where the buy conditions hold."""
        dataframe.loc[
            (
                (dataframe["rsi"] < 30)                    # oversold
                & (dataframe["close"] > dataframe["ema50"])  # still in an uptrend
                & (dataframe["volume"] > 0)                  # tradable candle
            ),
            "enter_long",
        ] = 1
        return dataframe

    def populate_exit_trend(self, dataframe: DataFrame, metadata: dict) -> DataFrame:
        """Set exit_long = 1 on rows where the sell condition holds."""
        dataframe.loc[
            (
                (dataframe["rsi"] > 70)      # overbought
                & (dataframe["volume"] > 0)
            ),
            "exit_long",
        ] = 1
        return dataframe

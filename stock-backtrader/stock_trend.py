"""
Stock trend-filter backtest (backtrader + yfinance)
====================================================

Ports the crypto BTCETHTrend logic to US stocks/ETFs:
  hold while price > N-day SMA, move to cash when it drops below.
(Faber 2007's classic 200-day timing model — its original home is equities.)

Usage:
  python stock_trend.py --ticker SPY --start 2005-01-01 --sma 200
  python stock_trend.py --ticker QQQ --start 2010-01-01 --sma 200

Research/backtest only. Not investment advice.
"""

import argparse
import datetime as dt

import pandas as pd

# backtrader is unmaintained; it calls DataFrame.iteritems(), removed in pandas 2.0.
if not hasattr(pd.DataFrame, "iteritems"):
    pd.DataFrame.iteritems = pd.DataFrame.items

import backtrader as bt
import yfinance as yf


class TrendSMA(bt.Strategy):
    params = dict(sma_period=200)

    def __init__(self):
        self.sma = bt.indicators.SimpleMovingAverage(
            self.data.close, period=self.p.sma_period
        )
        self.trades = 0

    def next(self):
        price = self.data.close[0]
        if not self.position:
            if price > self.sma[0]:
                self.order_target_percent(target=0.95)  # go long (leave 5% for fees)
        else:
            if price < self.sma[0]:
                self.close()  # trend broke -> to cash

    def notify_trade(self, trade):
        if trade.isclosed:
            self.trades += 1


def load_data(ticker, start, end, monthly=False):
    df = yf.download(ticker, start=start, end=end, auto_adjust=True, progress=False)
    if df is None or df.empty:
        raise SystemExit(f"No data for {ticker}. Check ticker/date range/network.")
    # yfinance may return MultiIndex columns (('Close','SPY')) -> flatten to level 0.
    if isinstance(df.columns, pd.MultiIndex):
        df.columns = df.columns.get_level_values(0)
    df = df.rename(columns=str.lower)  # Open->open etc. for backtrader
    if monthly:
        # Faber's original: decide once per month on month-end closes.
        df = df.resample("ME").agg(
            {"open": "first", "high": "max", "low": "min", "close": "last", "volume": "sum"}
        ).dropna()
    return df


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ticker", default="SPY")
    ap.add_argument("--start", default="2005-01-01")
    ap.add_argument("--end", default=dt.date.today().isoformat())
    ap.add_argument("--sma", type=int, default=0,
                    help="SMA period in bars; default 200 (daily) or 10 (monthly)")
    ap.add_argument("--monthly", action="store_true",
                    help="Faber's original: month-end bars + 10-month SMA")
    ap.add_argument("--cash", type=float, default=10000)
    ap.add_argument("--commission", type=float, default=0.0005, help="per-side, 0.0005 = 0.05%")
    args = ap.parse_args()

    if args.sma == 0:
        args.sma = 10 if args.monthly else 200

    df = load_data(args.ticker, args.start, args.end, monthly=args.monthly)

    cerebro = bt.Cerebro()
    cerebro.addstrategy(TrendSMA, sma_period=args.sma)
    cerebro.adddata(bt.feeds.PandasData(dataname=df))
    cerebro.broker.setcash(args.cash)
    cerebro.broker.setcommission(commission=args.commission)

    cerebro.addanalyzer(bt.analyzers.DrawDown, _name="dd")
    cerebro.addanalyzer(bt.analyzers.TradeAnalyzer, _name="ta")
    cerebro.addanalyzer(bt.analyzers.SharpeRatio, _name="sharpe",
                        timeframe=bt.TimeFrame.Days, riskfreerate=0.0)

    start_val = cerebro.broker.getvalue()
    strat = cerebro.run()[0]
    end_val = cerebro.broker.getvalue()

    # Buy-and-hold benchmark over the same window.
    bh_ret = (df["close"].iloc[-1] / df["close"].iloc[0] - 1) * 100
    # Buy-and-hold max drawdown.
    roll_max = df["close"].cummax()
    bh_dd = ((df["close"] / roll_max - 1).min()) * 100

    strat_ret = (end_val / start_val - 1) * 100
    dd = strat.analyzers.dd.get_analysis()
    ta = strat.analyzers.ta.get_analysis()
    sharpe = strat.analyzers.sharpe.get_analysis().get("sharperatio")

    won = ta.get("won", {}).get("total", 0)
    total = ta.get("total", {}).get("closed", 0)
    winrate = (won / total * 100) if total else 0.0

    print("\n=================== RESULT ===================")
    print(f"Ticker            : {args.ticker}   ({args.start} -> {args.end})")
    print(f"Rule              : hold while price > SMA{args.sma}, else cash")
    print(f"Fee (per side)    : {args.commission*100:.3f}%")
    print("----------------------------------------------")
    print(f"Strategy return   : {strat_ret:+.1f}%")
    print(f"Buy & hold return : {bh_ret:+.1f}%")
    print("----------------------------------------------")
    print(f"Strategy max DD   : -{dd['max']['drawdown']:.1f}%")
    print(f"Buy & hold max DD : {bh_dd:.1f}%")
    print("----------------------------------------------")
    print(f"Trades            : {total}")
    print(f"Win rate          : {winrate:.1f}%")
    print(f"Sharpe (daily)    : {sharpe if sharpe is not None else 'n/a'}")
    print("==============================================")
    print("Backtest only. Past performance != future results.")


if __name__ == "__main__":
    main()

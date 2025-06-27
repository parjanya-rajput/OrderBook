#pragma once

#include "TradeInfo.h"

// Trade that actually happens between buyer and seller
struct Trade
{
public:
    Trade(const TradeInfo &bidTrade, const TradeInfo &askTrade)
    {
        bidTrade_ = bidTrade;
        askTrade_ = askTrade;
    }

    const TradeInfo &GetBidTrade() const { return bidTrade_; }
    const TradeInfo &GetAskTrade() const { return askTrade_; }

private:
    TradeInfo bidTrade_, askTrade_;
};

// We can have one buyer matched with multiple seller
using Trades = vector<Trade>;
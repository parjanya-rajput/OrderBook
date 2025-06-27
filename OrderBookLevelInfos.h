#pragma once

#include "LevelInfo.h"

// Class to represent the order book level information
// This class holds the bid and ask levels of an order book.
// It contains two vectors: one for bids and one for asks.
class OrderBookLevelInfos
{
public:
    OrderBookLevelInfos(const LevelInfos &bids, const LevelInfos &asks)
    {
        bids_ = bids;
        asks_ = asks;
    }

    const LevelInfos &GetBids() const { return bids_; }
    const LevelInfos &GetAsks() const { return asks_; }

private:
    LevelInfos bids_;
    LevelInfos asks_;
};
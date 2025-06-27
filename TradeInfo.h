#pragma once

#include "Usings.h"

// Matching the trades - Ask and Bid should match ~ to do that we will have an TradeInfoObject

// Individual Trade information
struct TradeInfo
{
    OrderId orderId_;
    Price price_;
    Quantity quantity_;
};
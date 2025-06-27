#pragma once

/* Enum class for OrderType
 This enum class defines the types of orders that can be placed in a trading system.
 It includes 5 types: GoodTillCancel, FillAndKill, Market, GoodForDay, and FillOrKill.
 - GoodTillCancel orders remain active until they are either filled or canceled.
 - FillAndKill orders are executed immediately and any unfilled portion is canceled.
 - FillOrKill orders are executed in whole i.e either fill 100% or cancel the order.
 - Market orders are executed at the best available price in the market or at market price (I just want to buy or sell anyhow)
 - GoodForDay orders are valid for the current trading day and will be canceled at the end of the day if not filled.
*/
enum class OrderType
{
    GoodTillCancel,
    FillAndKill,
    Market,
    GoodForDay,
    FillOrKill
};
// pre-processive directives ensures that header files are compiled only once
#pragma once
// Include necessary imports only
#include <map>
#include <thread>
#include <unordered_map>
#include <condition_variable>
#include <mutex>

#include "Usings.h"
#include "Order.h"
#include "OrderModify.h"
#include "OrderBookLevelInfos.h"
#include "Trade.h"

// OrderBook class to manage the order book
// This class is responsible for maintaining the order book, processing orders, and matching trades.
/* Requirements:
    - Easy O(1) access to orders based on OrderId
    - Easy O(1) access to best bid and ask price
*/
class OrderBook
{
private:
    // Represent Order and it's location in the order book
    struct OrderEntry
    {
        OrderPointer order_ = nullptr;
        OrderPointers::iterator location_;
    };

    // Relevant for FillOrKill orders
    struct LevelData
    {
        Quantity quantity_{};
        Quantity count_{};

        // Events that affect the level data
        enum class Action
        {
            Add,
            Remove,
            Match,
        };
    };

    // Meta data for each price level in the order book(useful for FillOrKill orders)
    unordered_map<Price, LevelData> data_;

    // Maps for asks and bids - sorting based on price so Price is the key -> Order
    // Bids are sorted in descending order (highest price first)
    // Asks in ascending order (lowest price first)
    map<Price, OrderPointers, greater<Price>> bids_;
    map<Price, OrderPointers, less<Price>> asks_;
    unordered_map<OrderId, OrderEntry> orders_;

    // Background thread that keeps running whole day at the end it prunes the GoodForDay orders
    thread ordersPruneThread_;
    mutable mutex ordersMutex_;
    condition_variable shutDownConditionVariable_;
    atomic<bool> shutDown_{false};

    // APIs that affect the state of the order book on specific events
    void onOrderCancelled(OrderPointer order);
    void onOrderAdded(OrderPointer order);
    void onOrderMatched(Price price, Quantity quantity, bool isFullFilled);
    void updateLevelData(Price price, Quantity quantity, LevelData::Action action);

    // Function to prune GoodForDay orders at the end of the day
    void CancelOrders(OrderIds orderIds);
    void CancelOrderInternal(OrderId orderId);

    bool canFullyFill(Side side, Price price, Quantity quantity) const;
    bool canMatch(Side side, Price price) const;
    void PruneGoodForDayOrders();
    Trades MatchOrders();

public:
    OrderBook();
    OrderBook(const OrderBook &) = delete;
    void operator=(const OrderBook &) = delete;
    OrderBook(OrderBook &&) = delete;
    void operator=(OrderBook &&) = delete;
    ~OrderBook();

    /*Add, Modify, Remove Order functions*/
    Trades AddOrder(OrderPointer order);
    void CancelOrder(OrderId orderId);
    Trades ModifyOrder(OrderModify order);
    size_t Size() const;
    OrderBookLevelInfos GetOrderBookLevelInfos() const;
};

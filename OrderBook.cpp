#include "OrderBook.h"

#include <numeric>
#include <ctime>
#include <chrono>

void OrderBook::PruneGoodForDayOrders()
{
    using namespace std::chrono;
    const auto end = hours(16); // Market closes at 4:00 PM

    while (true)
    {
        const auto now = system_clock::now();
        const auto now_c = system_clock::to_time_t(now);
        std::tm now_parts;
        localtime_r(&now_c, &now_parts);

        // If the current time is past the end of the trading day, we prune GoodForDay orders
        // But if it exceeds the end time, we wait for the next day
        if (now_parts.tm_hour >= end.count())
        {
            now_parts.tm_mday += 1; // Move to next day
        }

        now_parts.tm_hour = end.count();
        now_parts.tm_min = 0;
        now_parts.tm_sec = 0;

        auto next = system_clock::from_time_t(mktime(&now_parts));
        auto till = next - now + milliseconds(100); // Adding 100ms to ensure we don't miss the time window

        {
            // Lock the orders mutex to safely access the orders map
            std::unique_lock ordersLock{ordersMutex_};

            // Wait until the shutdown flag is set or the condition variable is notified
            if (shutDown_.load(std::memory_order_acquire) ||
                shutDownConditionVariable_.wait_for(ordersLock, till) == std::cv_status::timeout)
                return;
        }

        OrderIds orderIds;
        {
            std::scoped_lock ordersLock{ordersMutex_};

            for (const auto &[_, entry] : orders_)
            {
                const auto &[order, x] = entry;
                if (order->GetOrderType() != OrderType::GoodForDay)
                    continue;

                orderIds.push_back(order->GetOrderId());
            }
        }

        // Cancel all GoodForDay orders
        CancelOrders(orderIds);
    }
}

// Private API to cancel a list of orders
void OrderBook::CancelOrders(OrderIds orderIds)
{
    std::scoped_lock ordersLock{ordersMutex_};

    for (const auto &orderId : orderIds)
    {
        // Because of threading I didn't use CancelOrder(orderId) directly
        // As there will be O(n) lock taking and releasing simulataneously
        // So I created a separate function to cancel the order which takes the lock
        // and do all changes and releases the lock only once
        CancelOrderInternal(orderId);
    }
}

void OrderBook::CancelOrderInternal(OrderId orderId)
{
    // Check if the order exists in the orders map
    if (orders_.find(orderId) == orders_.end())
        return;

    const auto [order, orderIterator] = orders_.at(orderId);
    orders_.erase(orderId);
    // Here we will see the power of iterators
    if (order->GetSide() == Side::Sell)
    {
        auto price = order->GetPrice();
        auto &orders = asks_.at(price);
        // No need to traverse whole list, we can use the iterator to remove the order directly
        orders.erase(orderIterator);

        if (orders.empty())
        {
            asks_.erase(price);
        }
    }
    else
    {
        auto price = order->GetPrice();
        auto &orders = bids_.at(price);

        orders.erase(orderIterator);
        if (orders.empty())
        {
            bids_.erase(price);
        }
    }

    onOrderCancelled(order);
}

void OrderBook::onOrderCancelled(OrderPointer order)
{

    updateLevelData(order->GetPrice(), order->GetInitialQuantity(), LevelData::Action::Remove);
}
void OrderBook::onOrderMatched(Price price, Quantity quantity, bool isFullyFilled)
{

    updateLevelData(price, quantity, isFullyFilled ? LevelData::Action::Remove : LevelData::Action::Match);
}

void OrderBook::onOrderAdded(OrderPointer order)
{
    // Notify the system that an order has been added
    // This can be used for logging, analytics, or other purposes

    updateLevelData(order->GetPrice(), order->GetInitialQuantity(), LevelData::Action::Add);
}

void OrderBook::updateLevelData(Price price, Quantity quantity, LevelData::Action action)
{
    auto &data = data_[price];

    data.count_ += action == LevelData::Action::Remove ? -1 : action == LevelData::Action::Add ? 1
                                                                                               : 0;

    if (action == LevelData::Action::Remove || action == LevelData::Action::Match)
    {
        data.quantity_ -= quantity;
    }
    else
    {
        data.quantity_ += quantity;
    }

    if (data.count_ == 0)
    {
        data_.erase(price);
    }
}

bool OrderBook::canMatch(Side side, Price price) const
{
    if (side == Side::Buy)
    {
        if (asks_.empty())
            return false;
        // For Buy side, we need to check if the price is greater than or equal to
        // the lowest ask price (best ask).
        const auto &[bestAsk, _] = *asks_.begin();
        return price >= bestAsk;
    }
    else
    {
        if (bids_.empty())
            return false;
        // For Sell side, we need to check if the price is less than or equal to
        // the highest bid price (best bid).
        const auto &[bestBid, _] = *bids_.begin();
        return price <= bestBid;
    }
}

bool OrderBook::canFullyFill(Side side, Price price, Quantity quantity) const
{
    if (!canMatch(side, price))
        return false;

    std::optional<Price> threshold;
    if (side == Side::Buy)
    {
        const auto [askPrice, _] = *asks_.begin();
        threshold = askPrice;
    }
    else
    {
        const auto [bidPrice, _] = *bids_.begin();
        threshold = bidPrice;
    }

    for (const auto &[levelPrice, levelData] : data_)
    {
        if (threshold.has_value() &&
                (side == Side::Buy && threshold.value() > levelPrice) ||
            (side == Side::Sell && threshold.value() < levelPrice))
            continue;

        if ((side == Side::Buy && levelPrice > price) ||
            (side == Side::Sell && levelPrice < price))
            continue;

        if (quantity <= levelData.quantity_)
            return true;

        quantity -= levelData.quantity_;
    }

    return false;
}

// Match orders based on the current order book state
Trades OrderBook::MatchOrders()
{
    Trades trades;
    trades.reserve(orders_.size());

    while (true)
    {

        // This is the safe check for map of bids and asks orders not the orders themselves
        if (asks_.empty() || bids_.empty())
            break;

        // Get the best bid and ask prices
        auto &[bidPrice, bids] = *bids_.begin();
        auto &[askPrice, asks] = *asks_.begin();

        if (bidPrice < askPrice)
        {
            // No more matches possible
            break;
        }

        // Match the orders for bids and asks
        // Note: OrderPointers is a list of shared pointers to Order objects
        // and prices is the key in the map to the OrderPointers
        while (bids.size() > 0 && asks.size() > 0)
        {
            // Advantage of using list is that we can use front() & back() methods
            // Can also use queue
            auto bid = bids.front();
            auto ask = asks.front();

            // Check if the bid can match with the ask - suffice the minimum requirements
            Quantity quantity = min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());

            // Trade Done so update the quantity
            bid->Fill(quantity);
            ask->Fill(quantity);

            // Bid order is filled completely
            if (bid->IsFilled())
            {
                // If the bid order is filled, remove it from the bids map
                bids.pop_front();
                orders_.erase(bid->GetOrderId());
            }

            if (ask->IsFilled())
            {
                // If the ask order is filled, remove it from the asks map
                asks.pop_front();
                orders_.erase(ask->GetOrderId());
            }

            // Create a trade info object for the matched order
            TradeInfo bidTrade{bid->GetOrderId(), bid->GetPrice(), quantity};
            TradeInfo askTrade{ask->GetOrderId(), ask->GetPrice(), quantity};

            // push the whole trade information with ask and bid trade information
            trades.push_back(Trade{bidTrade, askTrade});

            // Notify the system that an order has been matched
            onOrderMatched(bid->GetPrice(), quantity, bid->IsFilled());
            onOrderMatched(bid->GetPrice(), quantity, ask->IsFilled());
        }

        // Now remove from the map if the list is empty
        if (bids.empty())
        {
            bids_.erase(bidPrice);
        }
        if (asks.empty())
        {
            asks_.erase(askPrice);
        }
    }

    // Check for FillAndKill orders
    if (!bids_.empty())
    {
        auto &[_, bids] = *bids_.begin();
        auto &order = bids.front();
        if (order->GetOrderType() == OrderType::FillAndKill)
        {
            CancelOrder(order->GetOrderId());
        }
    }

    if (!asks_.empty())
    {
        auto &[_, asks] = *asks_.begin();
        auto &order = asks.front();
        if (order->GetOrderType() == OrderType::FillAndKill)
        {
            CancelOrder(order->GetOrderId());
        }
    }

    return trades;
}

/*It starts a new thread when an OrderBook object is created.
That thread runs the PruneGoodForDayOrders() member function*/
OrderBook::OrderBook() : ordersPruneThread_{[this]
                                            { PruneGoodForDayOrders(); }} {}

OrderBook::~OrderBook()
{
    shutDown_.store(true, std::memory_order_release);
    shutDownConditionVariable_.notify_one();
    ordersPruneThread_.join();
}

Trades OrderBook::AddOrder(OrderPointer order)
{
    if (orders_.find(order->GetOrderId()) != orders_.end())
        return {};
    // Convert a market order to a limit order by specifying the best available price
    // And go on filling it
    if (order->GetOrderType() == OrderType::Market)
    {
        // Here I have used lowest price available for buy orders and highest price available for sell orders
        if (order->GetSide() == Side::Buy && !asks_.empty())
        {
            // Note- asks_ is sorted in ascending order
            const auto &[marketAsk, _] = *asks_.begin();
            order->ToGoodTillCancel(marketAsk);
        }
        else if (order->GetSide() == Side::Sell && !bids_.empty())
        {
            const auto &[marketBid, _] = *bids_.begin();
            order->ToGoodTillCancel(marketBid);
        }
        else
            return {};
    }
    if (order->GetOrderType() == OrderType::FillAndKill && !canMatch(order->GetSide(), order->GetPrice()))
    {
        // If the order is FillAndKill and cannot be matched, return empty trades
        return {};
    }

    if (order->GetOrderType() == OrderType::FillOrKill && !canFullyFill(order->GetSide(), order->GetPrice(), order->GetInitialQuantity()))
    {
        // If the order is FillOrKill and cannot be fully filled, return empty trades
        return {};
    }

    OrderPointers::iterator iterator;

    if (order->GetSide() == Side::Buy)
    {
        auto &orders = bids_[order->GetPrice()];
        orders.push_back(order);
        iterator = next(orders.begin(), orders.size() - 1);
    }
    else
    {
        auto &orders = asks_[order->GetPrice()];
        orders.push_back(order);
        iterator = next(orders.begin(), orders.size() - 1);
    }

    // Add the order to the orders map
    // orders_.insert({order->GetOrderId(), OrderEntry{order, iterator}});
    orders_[order->GetOrderId()] = OrderEntry{order, iterator};
    // Now match the orders

    // Bookkeeping events
    onOrderAdded(order);

    return MatchOrders();
}

// Cancel Order function
void OrderBook::CancelOrder(OrderId orderId)
{
    std::scoped_lock ordersLock{ordersMutex_};

    CancelOrderInternal(orderId);
}

Trades OrderBook::ModifyOrder(OrderModify order)
{
    if (orders_.find(order.GetOrderId()) == orders_.end())
        return {};

    const auto &[existingOrder, _] = orders_.at(order.GetOrderId());
    CancelOrder(order.GetOrderId());
    return AddOrder(order.ToOrderPointer(existingOrder->GetOrderType()));
}

size_t OrderBook::Size() const
{
    return orders_.size();
}

// Summarizes the current state of the order book by aggregating the total
// remaining quantity at each price level for both bids and asks.

OrderBookLevelInfos OrderBook::GetOrderBookLevelInfos() const
{
    LevelInfos bidInfos, askInfos;
    bidInfos.reserve(orders_.size());
    askInfos.reserve(orders_.size());

    auto CreateLevelInfos = [](Price price, const OrderPointers &orders)
    {
        return LevelInfo{price, accumulate(orders.begin(), orders.end(), (Quantity)0,
                                           [](Quantity runningSum, const OrderPointer &order)
                                           {
                                               return runningSum + order->GetRemainingQuantity();
                                           })};
    };

    for (const auto &[price, orders] : bids_)
    {
        bidInfos.push_back(CreateLevelInfos(price, orders));
    }
    for (const auto &[price, orders] : asks_)
    {
        askInfos.push_back(CreateLevelInfos(price, orders));
    }

    return OrderBookLevelInfos(bidInfos, askInfos);
}
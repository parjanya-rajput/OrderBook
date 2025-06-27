#pragma once

#include <list>
#include <exception>
#include <format>

#include "OrderType.h"
#include "Side.h"
#include "Usings.h"
#include "Constants.h"

class Order
{
private:
    OrderType orderType_;
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;

public:
    Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
    {
        orderType_ = orderType;
        orderId_ = orderId;
        side_ = side;
        price_ = price;
        initialQuantity_ = quantity;
        remainingQuantity_ = quantity;
    }

    // Constructor for market orders(we don't care about price here we just need to buy/sell)
    Order(OrderId orderId, Side side, Quantity quantity)
    {
        // We can use same constructor for market orders
        // by passing InvalidPrice as price and type as Market
        Order(OrderType::Market, orderId, side, -10, quantity);
    }

    // Public methods to access order details
    OrderType GetOrderType() const { return orderType_; }
    OrderId GetOrderId() const { return orderId_; }
    Side GetSide() const { return side_; }
    Price GetPrice() const { return price_; }
    Quantity GetInitialQuantity() const { return initialQuantity_; }
    Quantity GetRemainingQuantity() const { return remainingQuantity_; }
    Quantity GetFilledQuantity() const { return GetInitialQuantity() - GetRemainingQuantity(); }
    bool IsFilled() const { return GetRemainingQuantity() == 0; }
    // Public Methods to fill order
    void Fill(Quantity quantity)
    {
        // Safety check as we cannot take order with more quantity
        if (quantity > GetRemainingQuantity())
        {
            throw logic_error("Order (" + to_string(GetOrderId()) + "cannot be filled for more than remaining quantity");
        }

        // Actual filling starts here
        remainingQuantity_ -= quantity;
    }

    // For market orders we can convert them to GoodTillCancel orders
    //  by setting the price to the best available price in the order book
    //  we simply assign the price and change the order type
    void ToGoodTillCancel(Price price)
    {
        if (GetOrderType() != OrderType::Market)
        {
            throw std::logic_error(std::format("Order ({}) cannot have its price adjusted, only market orders can.", GetOrderId()));
        }

        price_ = price;
        orderType_ = OrderType::GoodTillCancel;
    }
};

// Stroing single order in multiple datastructure (stored in Orders dictionary & bid/ask based dictionary)
using OrderPointer = shared_ptr<Order>;
// Why list not vector - because it gives and iterator which cannot be invalidated despite list growing too large
// Also gives a simplicity
using OrderPointers = list<OrderPointer>;
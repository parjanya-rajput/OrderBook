#include <gtest/gtest.h>
#include "../OrderBook.h"

// Fill as much as possible, then reject the rest
TEST(OrderTypeTest, FillAndKillOrder) {
    OrderBook orderBook;
    
    // Add a sell order first
    auto sellOrder = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100.0, 5);
    orderBook.AddOrder(sellOrder);
    
    // Add FillAndKill buy order that can't be fully filled
    auto fakOrder = std::make_shared<Order>(OrderType::FillAndKill, 2, Side::Buy, 100.0, 10);
    auto trades = orderBook.AddOrder(fakOrder);
    
    EXPECT_FALSE(trades.empty()); // Partially filled so trade is not empty
    EXPECT_EQ(orderBook.Size(), 0); // Sell order is filled, so order book is empty and rest buy order is removed

}

TEST(OrderTypeTest, MarketOrder) {
    OrderBook orderBook;
    
    // Add a sell order
    auto sellOrder = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100.0, 10);
    orderBook.AddOrder(sellOrder);
    
    // Add market buy order
    auto marketOrder = std::make_shared<Order>(2, Side::Buy, 10);
    auto trades = orderBook.AddOrder(marketOrder);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(orderBook.Size(), 0);
}

TEST(OrderTypeTest, FillOrKillOrder) {
    OrderBook orderBook;
    
    // Add a sell order
    auto sellOrder = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100.0, 10);
    orderBook.AddOrder(sellOrder);
    
    // Add fill or kill buy order
    auto fokOrder = std::make_shared<Order>(OrderType::FillOrKill, 2, Side::Buy, 100.0, 20);
    auto trades = orderBook.AddOrder(fokOrder);
    
    EXPECT_TRUE(trades.empty()); // No trades is created because order is not fully filled
    EXPECT_EQ(orderBook.Size(), 1); // Buy order is not added to the order book because it is not fully filled so only sell order
}

TEST(OrderTypeTest, GoodTillCancelOrder) {
    OrderBook orderBook;
    
    // Add a sell order
    auto sellOrder = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100.0, 10);
    orderBook.AddOrder(sellOrder);
    
    // Add good till cancel buy order
    auto gtcOrder = std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Buy, 100.0, 10);
    auto trades = orderBook.AddOrder(gtcOrder);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(orderBook.Size(), 0); // Sell order is filled, so order book is empty and rest buy order is removed
}

// Good for day order testing is left

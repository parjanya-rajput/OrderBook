#include <gtest/gtest.h>
#include "../OrderBook.h"

TEST(MatchingTest, BasicMatching) {
    OrderBook orderBook;
    
    // Add buy order
    auto buyOrder = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 100.0, 10);
    auto trades1 = orderBook.AddOrder(buyOrder);
    EXPECT_TRUE(trades1.empty());
    
    // Add matching sell order
    auto sellOrder = std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Sell, 100.0, 10);
    auto trades2 = orderBook.AddOrder(sellOrder);
    
    EXPECT_EQ(trades2.size(), 1);
    EXPECT_EQ(orderBook.Size(), 0); // Both orders should be filled
}

TEST(MatchingTest, PartialMatching) {
    OrderBook orderBook;
    
    // Add buy order for 10 units
    auto buyOrder = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 100.0, 10);
    orderBook.AddOrder(buyOrder);
    
    // Add sell order for 5 units
    auto sellOrder = std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Sell, 100.0, 5);
    auto trades = orderBook.AddOrder(sellOrder);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(orderBook.Size(), 1); // Buy order still has 5 units remaining
    EXPECT_EQ(buyOrder->GetRemainingQuantity(), 5);
    EXPECT_TRUE(sellOrder->IsFilled());
}
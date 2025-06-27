#include <gtest/gtest.h>
#include "../OrderBook.h"

class OrderBookTest : public ::testing::Test {
protected:
    void SetUp() override {
        orderBook = std::make_unique<OrderBook>();
    }
    
    std::unique_ptr<OrderBook> orderBook;
};

TEST_F(OrderBookTest, AddOrder) {
    auto order = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 100.0, 10);
    auto trades = orderBook->AddOrder(order);
    
    EXPECT_EQ(orderBook->Size(), 1);
    EXPECT_TRUE(trades.empty());
}

TEST_F(OrderBookTest, CancelOrder) {
    auto order = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 100.0, 10);
    orderBook->AddOrder(order);
    
    orderBook->CancelOrder(1);
    EXPECT_EQ(orderBook->Size(), 0);
}

TEST_F(OrderBookTest, CancelNonExistentOrder) {
    EXPECT_NO_THROW(orderBook->CancelOrder(999));
}
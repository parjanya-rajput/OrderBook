#include <gtest/gtest.h>
#include "../Order.h"

TEST(OrderTest, Constructor) {
    Order order(OrderType::GoodTillCancel, 1, Side::Buy, 100.0, 10);
    EXPECT_EQ(order.GetOrderId(), 1);
    EXPECT_EQ(order.GetSide(), Side::Buy);
    EXPECT_EQ(order.GetPrice(), 100.0);
    EXPECT_EQ(order.GetInitialQuantity(), 10);
    EXPECT_EQ(order.GetRemainingQuantity(), 10);
}

TEST(OrderTest, FillOrder) {
    Order order(OrderType::GoodTillCancel, 1, Side::Buy, 100.0, 10);
    order.Fill(5);
    EXPECT_EQ(order.GetRemainingQuantity(), 5);
    EXPECT_EQ(order.GetFilledQuantity(), 5);
    EXPECT_FALSE(order.IsFilled());
}

TEST(OrderTest, FillOrderCompletely) {
    Order order(OrderType::GoodTillCancel, 1, Side::Buy, 100.0, 10);
    order.Fill(10);
    EXPECT_EQ(order.GetRemainingQuantity(), 0);
    EXPECT_TRUE(order.IsFilled());
}

TEST(OrderTest, FillOrderThrowsException) {
    Order order(OrderType::GoodTillCancel, 1, Side::Buy, 100.0, 10);
    EXPECT_THROW(order.Fill(15), std::logic_error);
}

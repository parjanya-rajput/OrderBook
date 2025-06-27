/*An orderbook is a Data structure partitioned by buys and sells such that buy orders and sell orders
are matched on price-time priority*/
#include <iostream>
#include "OrderBook.h"
using namespace std;

int main()
{
    OrderBook orderBook;
    const OrderId orderId = 1;
    auto cur = orderBook.AddOrder(make_shared<Order>(OrderType::GoodTillCancel, orderId, Side::Buy, 100.0, 10));
    cout << orderBook.Size() << endl; // Should print 1
    orderBook.CancelOrder(orderId);
    cout << orderBook.Size() << endl; // Should print 0
    return 0;
}

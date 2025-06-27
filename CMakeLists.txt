cmake_minimum_required(VERSION 3.16)
project(OrderBook)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Google Test
find_package(GTest REQUIRED)

# Main library
add_library(orderbook_lib
    Constants.h
    main.cpp
    OrderBook.cpp
    Order.h
    OrderBook.h
    OrderModify.h
    OrderBookLevelInfos.h
    Trade.h
    Usings.h
    Side.h
    OrderType.h
    LevelInfo.h
    TradeInfo.h
)

# Main executable
add_executable(orderbook main.cpp)
target_link_libraries(orderbook orderbook_lib)

# Tests
enable_testing()
add_subdirectory(tests)
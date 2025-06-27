# Test executable
add_executable(orderbook_tests
    test_main.cpp
    test_order.cpp
    test_orderbook.cpp
    test_matching.cpp
    test_order_types.cpp
    test_threading.cpp
)

target_link_libraries(orderbook_tests
    orderbook_lib
    GTest::gtest
    GTest::gtest_main
)

# Register tests
include(GoogleTest)
gtest_discover_tests(orderbook_tests)
find_package(GTest REQUIRED)

add_executable(SPSCQueueTests SPSCQueueTests.cpp)
target_link_libraries(SPSCQueueTests PRIVATE Concurrency GTest::gtest_main)
add_test(NAME SPSCQueueTests COMMAND SPSCQueueTests)

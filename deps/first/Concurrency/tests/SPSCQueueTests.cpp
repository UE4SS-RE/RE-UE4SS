#include <gtest/gtest.h>
#include <Concurrency/SPSCQueue.hpp>
#include <thread>
#include <vector>

using namespace RC::Concurrency;

TEST(SPSCQueue, RuntimeSizing)
{
    SPSCQueue<int> small(512);
    SPSCQueue<int> large(16384);

    // capacity() returns usable capacity (buffer_size - 1)
    EXPECT_EQ(small.capacity(), 511);
    EXPECT_EQ(large.capacity(), 16383);
    // buffer_size() returns actual buffer size
    EXPECT_EQ(small.buffer_size(), 512);
    EXPECT_EQ(large.buffer_size(), 16384);
}

TEST(SPSCQueue, BulkDequeue)
{
    SPSCQueue<int> q(128);

    for (int i = 0; i < 50; ++i)
    {
        EXPECT_TRUE(q.try_enqueue(i));
    }

    int items[30];
    size_t dequeued = q.dequeue_bulk(items, 30);

    EXPECT_EQ(dequeued, 30);
    for (int i = 0; i < 30; ++i)
    {
        EXPECT_EQ(items[i], i);
    }

    EXPECT_EQ(q.approx_size(), 20);
}

TEST(SPSCQueue, ProducerConsumer)
{
    SPSCQueue<int> q(1024);
    std::atomic<bool> done{false};

    std::thread producer([&]() {
        for (int i = 0; i < 10000; ++i)
        {
            while (!q.try_enqueue(i))
            {
                std::this_thread::yield();
            }
        }
        done = true;
    });

    std::thread consumer([&]() {
        int count = 0;
        while (!done || q.approx_size() > 0)
        {
            int val;
            if (q.try_dequeue(val))
            {
                EXPECT_EQ(val, count++);
            }
        }
        EXPECT_EQ(count, 10000);
    });

    producer.join();
    consumer.join();
}

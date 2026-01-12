#pragma once
#include <atomic>
#include <chrono>
class QueueProfiler
{
public:
    static void BeginEnqueue()
    {
        enqueue_count.fetch_add(1, std::memory_order_relaxed);
        enqueue_start = std::chrono::high_resolution_clock::now();
    }

    static void EndEnqueue()
    {
        enqueue_total.fetch_add(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - enqueue_start).count(), std::memory_order_relaxed);
    }

    static double GetEnqueueAverage()
    {
        return (enqueue_total.load(std::memory_order_relaxed) / static_cast<double>(enqueue_count.load(std::memory_order_relaxed)));
    }

    static void BeginDequeue()
    {
        dequeue_count.fetch_add(1, std::memory_order_relaxed);
        dequeue_start = std::chrono::high_resolution_clock::now();
    }

    static void EndDequeue()
    {
        const auto now = std::chrono::high_resolution_clock::now();
        dequeue_total.fetch_add(std::chrono::duration_cast<std::chrono::microseconds>(now - dequeue_start).count(), std::memory_order_relaxed);
    }

    static double GetDequeueAverage()
    {
        return (dequeue_total.load(std::memory_order_relaxed) / static_cast<double>(dequeue_count.load(std::memory_order_relaxed)));
    }

    static void AddPendingCount(const int64_t pending)
    {
        pending_count.fetch_add(1, std::memory_order_relaxed);
        pending_total.fetch_add(pending, std::memory_order_relaxed);
    }

    static double GetPendingAverage()
    {
        return (pending_total.load(std::memory_order_relaxed) / static_cast<double>(pending_count.load(std::memory_order_relaxed)));
    }

    static void Reset()
    {
        enqueue_count.store(0, std::memory_order_release);
        enqueue_total.store(0, std::memory_order_release);
        dequeue_count.store(0, std::memory_order_release);
        dequeue_total.store(0, std::memory_order_release);
        pending_total.store(0, std::memory_order_release);
        pending_count.store(0, std::memory_order_release);
    }
private:
    inline static std::atomic_int64_t enqueue_total = 0;
    inline static std::atomic_int64_t dequeue_total = 0;
    inline static std::atomic_int64_t pending_total = 0;

    inline static std::atomic_int64_t enqueue_count = 0;
    inline static std::atomic_int64_t dequeue_count = 0;
    inline static std::atomic_int64_t pending_count = 0;

    inline static thread_local std::chrono::time_point<std::chrono::high_resolution_clock> enqueue_start;
    inline static thread_local std::chrono::time_point<std::chrono::high_resolution_clock> dequeue_start;
};
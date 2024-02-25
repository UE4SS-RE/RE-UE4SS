#pragma once

#include <atomic>
#include <optional>

namespace RC::Input 
{
    /// SAFETY: This is ONLY a single producer, single consumer lock-free queue
    template <typename T, int max_buffer = 256>
    struct RingBufferSPSC
    {
        T m_queue[max_buffer];
        std::atomic<unsigned int> m_head{0};
        std::atomic<unsigned int> m_tail{0};

        // tail belongs to the producer
        auto push(const T& event) -> bool
        {
            auto current_tail = m_tail.load(std::memory_order_relaxed);
            int next_tail = (current_tail + 1) % max_buffer;
            if (next_tail == m_head.load(std::memory_order_acquire))
            {
                // Queue is full
                return false;
            }

            m_queue[current_tail] = event;
            m_tail.store(next_tail, std::memory_order_release);
            return true;
        }

        // head belongs to the consumer
        auto pop() -> std::optional<T>
        {
            auto current_head = m_head.load(std::memory_order_relaxed);
            if (current_head == m_tail.load(std::memory_order_acquire))
            {
                // Queue is empty
                return std::nullopt;
            }

            T event = m_queue[current_head];
            m_head.store((current_head + 1) % max_buffer, std::memory_order_release);
            return event;
        }
    };
} // RC::Input


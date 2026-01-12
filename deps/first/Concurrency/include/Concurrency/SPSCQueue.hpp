#pragma once

#include <atomic>
#include <bit>
#include <vector>

namespace RC::Concurrency
{

    template <typename T>
    class alignas(64) SPSCQueue
    {
        static_assert(std::is_move_constructible_v<T>);
        static_assert(std::is_move_assignable_v<T>);

      private:
        alignas(64) std::atomic<size_t> m_head{0};
        static_assert(sizeof(std::atomic<size_t>) < 64, "Padding logic assumes std::atomic<size_t> < 64 bytes");
        char _pad_head[64 - sizeof(std::atomic<size_t>)];

        alignas(64) std::atomic<size_t> m_tail{0};
        static_assert(sizeof(std::atomic<size_t>) < 64, "Padding logic assumes std::atomic<size_t> < 64 bytes");
        char _pad_tail[64 - sizeof(std::atomic<size_t>)];

        std::vector<T> m_buffer;
        size_t m_capacity_mask;

      public:
        explicit SPSCQueue(size_t capacity)
        {
            size_t sane_capacity = std::max(capacity, static_cast<size_t>(2));
            size_t rounded = std::bit_ceil(sane_capacity);
            m_buffer.resize(rounded);
            m_capacity_mask = rounded - 1;
        }

        auto try_enqueue(const T& item) -> bool
        {
            size_t head = m_head.load(std::memory_order_relaxed);
            size_t next_head = (head + 1) & m_capacity_mask;

            if (next_head == m_tail.load(std::memory_order_acquire))
            {
                return false;
            }

            m_buffer[head] = item;
            m_head.store(next_head, std::memory_order_release);
            return true;
        }

        auto try_enqueue(T&& item) -> bool
        {
            size_t head = m_head.load(std::memory_order_relaxed);
            size_t next_head = (head + 1) & m_capacity_mask;

            if (next_head == m_tail.load(std::memory_order_acquire))
            {
                return false;
            }

            m_buffer[head] = std::move(item);
            m_head.store(next_head, std::memory_order_release);
            return true;
        }

        template <typename... Args>
        auto try_emplace(Args&&... args) -> bool
        {
            size_t head = m_head.load(std::memory_order_relaxed);
            size_t next_head = (head + 1) & m_capacity_mask;

            if (next_head == m_tail.load(std::memory_order_acquire))
            {
                return false;
            }

            m_buffer[head] = T(std::forward<Args>(args)...);
            m_head.store(next_head, std::memory_order_release);
            return true;
        }

        auto try_dequeue(T& item) -> bool
        {
            size_t tail = m_tail.load(std::memory_order_relaxed);

            if (tail == m_head.load(std::memory_order_acquire))
            {
                return false;
            }

            item = std::move(m_buffer[tail]);
            m_tail.store((tail + 1) & m_capacity_mask, std::memory_order_release);
            return true;
        }

        /// Dequeue up to max_count items into out array.
        /// Note: Items in the queue are left in a moved-from state after dequeue.
        /// The tail pointer is updated atomically after all items are moved,
        /// so if the consumer is interrupted mid-operation, the items remain
        /// logically in the queue (though in moved-from state). This is safe
        /// for SPSC as long as moved-from T is valid (which is required by the
        /// static_assert at class level). For types where moved-from state is
        /// problematic, use try_dequeue() in a loop instead.
        auto dequeue_bulk(T* out, size_t max_count) -> size_t
        {
            size_t tail = m_tail.load(std::memory_order_relaxed);
            size_t head = m_head.load(std::memory_order_acquire);

            if (tail == head)
            {
                return 0;
            }

            size_t available = (head >= tail) ? (head - tail) : (m_capacity_mask + 1 - tail + head);

            size_t to_dequeue = std::min(available, max_count);
            size_t new_tail = tail;

            for (size_t i = 0; i < to_dequeue; ++i)
            {
                out[i] = std::move(m_buffer[new_tail]);
                new_tail = (new_tail + 1) & m_capacity_mask;
            }

            m_tail.store(new_tail, std::memory_order_release);
            return to_dequeue;
        }

        /// Returns approximate number of items in the queue.
        /// Note: This is inherently racy in SPSC - between reading head and tail,
        /// values can change. The result is a point-in-time estimate, not a
        /// guarantee. Use only for heuristics (e.g., deciding when to wake consumer).
        auto approx_size() const -> size_t
        {
            size_t head = m_head.load(std::memory_order_relaxed);
            size_t tail = m_tail.load(std::memory_order_relaxed);
            return (head >= tail) ? (head - tail) : (m_capacity_mask + 1 - tail + head);
        }

        /// Returns the maximum number of items that can be stored.
        /// Note: Due to ring buffer semantics (head==tail means empty),
        /// usable capacity is buffer_size - 1.
        auto capacity() const -> size_t
        {
            return m_capacity_mask;
        }

        /// Returns the underlying buffer size (capacity + 1).
        auto buffer_size() const -> size_t
        {
            return m_capacity_mask + 1;
        }
    };

} // namespace RC::Concurrency

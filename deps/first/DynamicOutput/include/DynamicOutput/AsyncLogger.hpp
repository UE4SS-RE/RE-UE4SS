#pragma once

#include <Concurrency/SPSCQueue.hpp>
#include <DynamicOutput/OutputDevice.hpp>
#include <String/StringType.hpp>
#include <array>
#include <chrono>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string_view>
#include <thread>
#include <vector>

namespace RC::Output
{

    struct LogEntry
    {
        static constexpr size_t INLINE_CAPACITY = 96;

        std::chrono::system_clock::time_point timestamp;
        int32_t log_level;
        std::thread::id thread_id;
        size_t sequence_number;

        std::array<CharType, INLINE_CAPACITY + 1> inline_buffer;
        std::unique_ptr<CharType[]> heap_buffer;
        uint32_t message_length;
        bool is_heap_allocated;

        LogEntry() = default;

        LogEntry(LogEntry&& other) noexcept
            : timestamp(other.timestamp), log_level(other.log_level), thread_id(other.thread_id), sequence_number(other.sequence_number),
              message_length(other.message_length), is_heap_allocated(other.is_heap_allocated)
        {
            // Copy inline buffer BEFORE moving heap_buffer, while other is still valid
            if (!other.is_heap_allocated && other.message_length > 0)
            {
                std::memcpy(inline_buffer.data(), other.inline_buffer.data(), (other.message_length + 1) * sizeof(CharType));
            }
            else
            {
                heap_buffer = std::move(other.heap_buffer);
            }

            other.message_length = 0;
            other.is_heap_allocated = false;
        }

        auto operator=(LogEntry&& other) noexcept -> LogEntry&
        {
            if (this != &other)
            {
                timestamp = other.timestamp;
                log_level = other.log_level;
                thread_id = other.thread_id;
                sequence_number = other.sequence_number;
                message_length = other.message_length;
                is_heap_allocated = other.is_heap_allocated;

                // Copy inline buffer BEFORE moving heap_buffer, while other is still valid
                if (!other.is_heap_allocated && other.message_length > 0)
                {
                    std::memcpy(inline_buffer.data(), other.inline_buffer.data(), (other.message_length + 1) * sizeof(CharType));
                }
                else
                {
                    heap_buffer = std::move(other.heap_buffer);
                }

                other.message_length = 0;
                other.is_heap_allocated = false;
            }
            return *this;
        }

        auto get_message() const -> StringViewType
        {
            if (is_heap_allocated)
            {
                return StringViewType{heap_buffer.get(), message_length};
            }
            return StringViewType{inline_buffer.data(), message_length};
        }

        auto set_message(StringViewType msg) -> void
        {
            message_length = static_cast<uint32_t>(msg.length());

            if (message_length <= INLINE_CAPACITY)
            {
                std::copy_n(msg.data(), message_length, inline_buffer.data());
                inline_buffer[message_length] = CharType{0};
                is_heap_allocated = false;
                heap_buffer.reset();
            }
            else
            {
                heap_buffer = std::make_unique<CharType[]>(message_length + 1);
                std::copy_n(msg.data(), message_length, heap_buffer.get());
                heap_buffer[message_length] = CharType{0};
                is_heap_allocated = true;
            }
        }
    };

    class AsyncLogger;

    struct ThreadQueue
    {
        Concurrency::SPSCQueue<LogEntry> queue;
        std::thread::id thread_id;
        std::atomic<size_t> enqueue_count{0};
        std::atomic<size_t> drop_count{0};

        explicit ThreadQueue(size_t queue_size) : queue(queue_size)
        {
        }
    };

    class AsyncLogger : public std::enable_shared_from_this<AsyncLogger>
    {
      private:
        std::vector<std::unique_ptr<ThreadQueue>> m_queues;
        mutable std::shared_mutex m_queues_mutex;

        // Queue of thread IDs requesting their queue be drained and removed.
        // Worker thread processes these to maintain single-consumer invariant.
        Concurrency::SPSCQueue<std::thread::id> m_exit_requests{64};

        std::jthread m_worker_thread;
        std::atomic<bool> m_running{false};

        std::atomic<uint64_t> m_generation{1};

        std::atomic<uint64_t> m_loop_counter{0};
        uint64_t m_last_flush_loop{0};

        uint64_t m_flush_every_n_loops{10};
        size_t m_batch_size{1024};
        size_t m_batch_wake_threshold{512};
        std::chrono::milliseconds m_flush_interval{100};

        size_t m_default_queue_size{2048};

        std::atomic<size_t> m_total_enqueued{0};
        std::atomic<size_t> m_total_flushed{0};
        std::atomic<size_t> m_total_dropped{0};

        std::atomic<bool> m_nudge{false};

        // Flush synchronization: caller sets request, worker acknowledges
        std::atomic<bool> m_flush_requested{false};
        std::atomic<bool> m_flush_completed{false};

        size_t m_next_queue_index{0};

        std::array<LogEntry, 128> m_dequeue_buffer;

      public:
        AsyncLogger() = default;
        ~AsyncLogger();

        auto start(uint64_t flush_every_n_loops,
                   size_t batch_size,
                   size_t batch_wake_threshold,
                   std::chrono::milliseconds flush_interval,
                   size_t default_queue_size = 2048) -> void;

        auto stop() -> void;

        auto try_enqueue(LogEntry&& entry) -> bool;

        auto on_loop_tick() -> void;

        auto flush() -> void;

        auto get_total_enqueued() const -> size_t
        {
            return m_total_enqueued.load();
        }
        auto get_total_flushed() const -> size_t
        {
            return m_total_flushed.load();
        }
        auto get_total_dropped() const -> size_t
        {
            return m_total_dropped.load();
        }
        auto get_approx_pending() const -> size_t;

        auto notify_thread_exit(std::thread::id tid, uint64_t generation) -> void;

        static void set_current_thread_queue_size(size_t size);

        static constexpr size_t QUEUE_SIZE_SMALL = 2048;
        static constexpr size_t QUEUE_SIZE_MEDIUM = 8192;
        static constexpr size_t QUEUE_SIZE_LARGE = 16384;
        static constexpr size_t QUEUE_SIZE_XLARGE = 32768;

      private:
        auto get_or_create_thread_queue() -> ThreadQueue*;
        auto worker_loop(std::stop_token stoken) -> void;
        auto flush_batch(LogEntry* entries, size_t count) -> void;
        auto drain_all_queues() -> void;
        auto process_exit_requests() -> void;
    };

} // namespace RC::Output

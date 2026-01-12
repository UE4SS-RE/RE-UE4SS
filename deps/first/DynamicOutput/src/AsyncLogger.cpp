#include <DynamicOutput/AsyncLogger.hpp>
#include <DynamicOutput/FileDevice.hpp>
#include <DynamicOutput/Output.hpp>
#include <algorithm>

#ifdef _MSC_VER
#include <intrin.h>
#define CPU_PAUSE() _mm_pause()
#else
#define CPU_PAUSE() __builtin_ia32_pause()
#endif

namespace RC::Output {

struct ThreadLocalState {
    ThreadQueue* queue{nullptr};
    uint64_t generation{0};
    size_t custom_queue_size{0};
    std::weak_ptr<AsyncLogger> logger_weak;
    std::thread::id thread_id;

    ~ThreadLocalState() {
        if (auto logger_sp = logger_weak.lock()) {
            logger_sp->notify_thread_exit(thread_id, generation);
        }
    }
};

AsyncLogger::~AsyncLogger()
{
    stop();
}

auto AsyncLogger::start(uint64_t flush_every_n_loops,
                        size_t batch_size,
                        size_t batch_wake_threshold,
                        std::chrono::milliseconds flush_interval,
                        size_t default_queue_size) -> void
{
    m_flush_every_n_loops = flush_every_n_loops;
    m_batch_size = batch_size;
    m_batch_wake_threshold = batch_wake_threshold;
    m_flush_interval = flush_interval;
    m_default_queue_size = default_queue_size;
    m_running.store(true, std::memory_order_release);

    m_worker_thread = std::jthread([this](std::stop_token stoken) {
        worker_loop(stoken);
    });
}

auto AsyncLogger::stop() -> void
{
    if (!m_running.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    m_generation.fetch_add(1, std::memory_order_release);

    if (m_worker_thread.joinable()) {
        m_worker_thread.request_stop();
        m_worker_thread.join();
    }

    drain_all_queues();
}

namespace {
    thread_local ThreadLocalState tls_state;
}

auto AsyncLogger::set_current_thread_queue_size(size_t size) -> void {
    if (tls_state.queue) {
        return;
    }
    tls_state.custom_queue_size = size;
}

auto AsyncLogger::try_enqueue(LogEntry&& entry) -> bool
{
    uint64_t current_gen = m_generation.load(std::memory_order_acquire);
    if (tls_state.generation != current_gen) {
        tls_state.queue = nullptr;
        tls_state.generation = current_gen;
    }

    if (!tls_state.queue) {
        tls_state.queue = get_or_create_thread_queue();
        tls_state.logger_weak = shared_from_this();
        tls_state.thread_id = std::this_thread::get_id();
    }

    entry.sequence_number = m_total_enqueued.fetch_add(1, std::memory_order_relaxed);

    constexpr int MAX_RETRIES = 5;
    constexpr int MAX_SPINS = 256;

    for (int retry = 0; retry < MAX_RETRIES; ++retry) {
        if (tls_state.queue->queue.try_enqueue(std::move(entry))) {
            tls_state.queue->enqueue_count.fetch_add(1, std::memory_order_relaxed);

            if (tls_state.queue->queue.approx_size() > m_batch_wake_threshold / 2) {
                m_nudge.store(true, std::memory_order_relaxed);
            }

            return true;
        }

        int spins = std::min(MAX_SPINS, 1 << retry);
        for (int i = 0; i < spins; ++i) {
            CPU_PAUSE();
        }

        if (retry >= 2) {
            std::this_thread::yield();
        }
    }

    tls_state.queue->drop_count.fetch_add(1, std::memory_order_relaxed);
    m_total_dropped.fetch_add(1, std::memory_order_relaxed);
    m_nudge.store(true, std::memory_order_relaxed);
    return false;
}

auto AsyncLogger::on_loop_tick() -> void
{
    m_loop_counter.fetch_add(1, std::memory_order_relaxed);
}

auto AsyncLogger::flush() -> void
{
    drain_all_queues();
}

auto AsyncLogger::notify_thread_exit(std::thread::id tid, uint64_t generation) -> void
{
    if (generation != m_generation.load(std::memory_order_acquire)) {
        return;
    }

    std::vector<LogEntry> remaining_entries;
    remaining_entries.reserve(m_default_queue_size);

    {
        std::unique_lock lock{m_queues_mutex};
        auto it = std::find_if(m_queues.begin(), m_queues.end(),
            [tid](const auto& tq) { return tq->thread_id == tid; });

        if (it != m_queues.end()) {
            LogEntry temp_buffer[256];
            size_t dequeued;
            while ((dequeued = (*it)->queue.dequeue_bulk(temp_buffer, 256)) > 0) {
                for (size_t i = 0; i < dequeued; ++i) {
                    remaining_entries.push_back(std::move(temp_buffer[i]));
                }
            }
            m_queues.erase(it);
        }
    }

    if (!remaining_entries.empty()) {
        flush_batch(remaining_entries.data(), remaining_entries.size());
        m_total_flushed.fetch_add(remaining_entries.size(), std::memory_order_relaxed);
    }
}

auto AsyncLogger::get_or_create_thread_queue() -> ThreadQueue*
{
    auto thread_id = std::this_thread::get_id();

    {
        std::shared_lock lock{m_queues_mutex};
        for (auto& tq : m_queues) {
            if (tq->thread_id == thread_id) {
                return tq.get();
            }
        }
    }

    std::unique_lock lock{m_queues_mutex};

    for (auto& tq : m_queues) {
        if (tq->thread_id == thread_id) {
            return tq.get();
        }
    }

    size_t queue_size = (tls_state.custom_queue_size > 0) ? tls_state.custom_queue_size : m_default_queue_size;
    auto tq = std::make_unique<ThreadQueue>(queue_size);
    tq->thread_id = thread_id;
    auto* ptr = tq.get();
    m_queues.push_back(std::move(tq));

    return ptr;
}

auto AsyncLogger::worker_loop(std::stop_token stoken) -> void
{
    std::vector<LogEntry> buffer;
    buffer.reserve(m_batch_size);
    auto last_flush_time = std::chrono::steady_clock::now();

    while (!stoken.stop_requested() && m_running.load(std::memory_order_relaxed)) {
        uint64_t current_loop = m_loop_counter.load(std::memory_order_relaxed);
        uint64_t loops_since_flush = current_loop - m_last_flush_loop;
        size_t total_pending = get_approx_pending();
        auto now = std::chrono::steady_clock::now();
        auto time_since_flush = now - last_flush_time;

        bool nudged = m_nudge.exchange(false, std::memory_order_acq_rel);

        bool should_flush = nudged ||
                           (loops_since_flush >= m_flush_every_n_loops) ||
                           (time_since_flush >= m_flush_interval) ||
                           (total_pending >= m_batch_wake_threshold);

        if (!should_flush) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        buffer.clear();
        {
            std::shared_lock lock{m_queues_mutex};

            size_t num_queues = m_queues.size();
            if (num_queues > 0) {
                for (size_t i = 0; i < num_queues && buffer.size() < m_batch_size; ++i) {
                    size_t queue_idx = (m_next_queue_index + i) % num_queues;
                    auto& tq = m_queues[queue_idx];

                    while (buffer.size() < m_batch_size) {
                        size_t dequeued = tq->queue.dequeue_bulk(m_dequeue_buffer.data(),
                            std::min<size_t>(m_dequeue_buffer.size(), m_batch_size - buffer.size()));

                        if (dequeued == 0) break;

                        for (size_t j = 0; j < dequeued; ++j) {
                            buffer.push_back(std::move(m_dequeue_buffer[j]));
                        }
                    }
                }
                m_next_queue_index = (m_next_queue_index + 1) % num_queues;
            }
        }

        if (!buffer.empty()) {
            flush_batch(buffer.data(), buffer.size());
            m_total_flushed.fetch_add(buffer.size(), std::memory_order_relaxed);
            m_last_flush_loop = current_loop;
            last_flush_time = now;
        }
    }
}

auto AsyncLogger::flush_batch(LogEntry* entries, size_t count) -> void
{
    if (count == 0) {
        return;
    }

    DefaultTargets::with_devices_read([&](const auto& devices) {
        for (const auto& device : devices) {
            if (auto* file_device = dynamic_cast<FileDevice*>(device.get())) {
                size_t total_chars = 0;
                for (size_t i = 0; i < count; ++i) {
                    total_chars += entries[i].message_length + 1;
                }

                StringType combined_output;
                combined_output.reserve(total_chars);

                for (size_t i = 0; i < count; ++i) {
                    auto msg = entries[i].get_message();
                    combined_output.append(msg);
                    if (!msg.ends_with(STR('\n'))) {
                        combined_output += STR('\n');
                    }
                }

                file_device->receive(combined_output);
            } else {
                for (size_t i = 0; i < count; ++i) {
                    auto msg = entries[i].get_message();
                    if (device->has_optional_arg()) {
                        device->receive_with_optional_arg(msg, entries[i].log_level);
                    } else {
                        device->receive(msg);
                    }
                }
            }
        }
    });
}

auto AsyncLogger::drain_all_queues() -> void
{
    std::vector<LogEntry> batch;

    std::shared_lock lock{m_queues_mutex};
    LogEntry temp_buffer[256];
    for (auto& tq : m_queues) {
        size_t dequeued;
        while ((dequeued = tq->queue.dequeue_bulk(temp_buffer, 256)) > 0) {
            for (size_t i = 0; i < dequeued; ++i) {
                batch.push_back(std::move(temp_buffer[i]));
            }
        }
    }
    lock.unlock();

    if (!batch.empty()) {
        flush_batch(batch.data(), batch.size());
        m_total_flushed.fetch_add(batch.size(), std::memory_order_relaxed);
    }
}

auto AsyncLogger::get_approx_pending() const -> size_t
{
    size_t total = 0;
    std::shared_lock lock{m_queues_mutex};
    for (const auto& tq : m_queues) {
        total += tq->queue.approx_size();
    }
    return total;
}

}

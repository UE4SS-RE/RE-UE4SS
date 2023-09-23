#ifndef UE4SS_REWRITTEN_SCOPEDTIMER_HPP
#define UE4SS_REWRITTEN_SCOPEDTIMER_HPP

#include <chrono>

namespace RC
{
    class ScopedTimer
    {
    private:
        std::chrono::time_point<std::chrono::steady_clock> m_start;
        double* m_dur_storage;
        bool m_timer_stopped{};

    public:
        ScopedTimer(double* dur_storage) : m_start(std::chrono::steady_clock::now()), m_dur_storage(dur_storage) {}

        ~ScopedTimer()
        {
            stop_timer();
        }

    public:
        auto stop_timer() -> void
        {
            if (!m_timer_stopped)
            {
                std::chrono::time_point <std::chrono::steady_clock> end = std::chrono::steady_clock::now();
                std::chrono::duration<double> a = end - m_start;
                *m_dur_storage = a.count();
                m_timer_stopped = true;
            }
        }
    };
}


#endif //UE4SS_REWRITTEN_SCOPEDTIMER_HPP

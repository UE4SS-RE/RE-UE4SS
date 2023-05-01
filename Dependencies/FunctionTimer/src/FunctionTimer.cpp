#include <Timer/FunctionTimer.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>

namespace RC
{
#if TIME_FUNCTION_MACRO_V2 == 0
    FunctionTimer::FunctionTimer(std::wstring_view function_name) : m_start(std::chrono::steady_clock::now()), m_function_name(function_name) {}
    FunctionTimer::FunctionTimer(std::string_view function_name) : m_start(std::chrono::steady_clock::now()), m_function_name(to_wstring(function_name)) {}

    FunctionTimer::~FunctionTimer()
    {
        std::chrono::time_point<std::chrono::steady_clock> end = std::chrono::steady_clock::now();
        std::chrono::duration<double> duration = end - m_start;
        FunctionTimerCollection::add_timer(this, duration.count());
    }

    auto FunctionTimer::get_function_name() -> std::wstring
    {
        return m_function_name;
    }

    auto FunctionTimerCollection::add_timer(FunctionTimer* timer, double duration) -> void
    {
        // Copy the timer to permanent storage
        // The original will get destroyed after its destructor finishes executing
        m_timers.emplace_back(SavedFunctionTimer{
            .function_name = timer->get_function_name(),
            .duration = duration
        });
    }

    auto FunctionTimerCollection::dump() -> void
    {
#ifdef TIME_FUNCTION_MACRO_ENABLED
        Output::Targets<Output::NewFileDevice, Output::DebugConsoleDevice> scoped_out;
        auto& file_device = scoped_out.get_device<Output::NewFileDevice>();
        file_device.set_file_name_and_path(STR("TimedFunctions.txt"));

        scoped_out.send(STR("Duration is in seconds\n"));
        scoped_out.send(STR("Scientific notation is likely a tiny number\n\n"));

        for (const auto& timer : m_timers)
        {
            scoped_out.send(STR("{} took {}\n"), timer.function_name, timer.duration);
        }
#endif
    }
#else // TIME_FUNCTION_MACRO_V2 == 0
    std::vector<FunctionTimerFrame> FunctionTimerFrame::s_frame_stack{};
    std::unordered_map<std::string, size_t> FunctionTimerFrame::s_functions_on_stack{};
    bool FunctionTimerFrame::s_timer_enabled{};
#endif // TIME_FUNCTION_MACRO_V2 == 0
}

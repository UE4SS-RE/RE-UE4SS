#ifndef RC_FUNCTION_TIMER_HPP
#define RC_FUNCTION_TIMER_HPP

#include <vector>
#include <unordered_map>
#include <chrono>
#include <numeric>

#include <Timer/Common.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>

#define TIME_FUNCTION_MACRO_V2 1

namespace RC
{
#if TIME_FUNCTION_MACRO_V2 == 0
    class RC_FNCTMR_API FunctionTimer
    {
    private:
        std::chrono::time_point<std::chrono::steady_clock> m_start;
        std::wstring m_function_name;

    public:
        FunctionTimer(std::wstring_view function_name);
        FunctionTimer(std::string_view function_name);
        ~FunctionTimer();

    public:
        auto get_function_name() -> std::wstring;
    };

    struct SavedFunctionTimer
    {
        const std::wstring function_name;
        const double duration{-1};
    };

    class RC_FNCTMR_API FunctionTimerCollection
    {
    private:
        static inline std::vector<SavedFunctionTimer> m_timers;

    public:
        auto static add_timer(FunctionTimer* timer, double duration) -> void;
        auto static dump() -> void;
    };

#if defined(TIME_FUNCTION_MACRO_ENABLED)
#ifndef TIME_FUNCTION
#define TIME_FUNCTION() \
FunctionTimer scoped_function_timer{__FUNCSIG__};
#endif
#else
#define TIME_FUNCTION()
#endif
#else

    using FunctionTimerIntervalUnderlyingType = double;
    using FunctionTimerInterval = std::chrono::duration<FunctionTimerIntervalUnderlyingType, std::micro>;
    using FunctionTimerClockType = std::chrono::system_clock;

    struct RC_FNCTMR_API FunctionTimerInternalFrame
    {
        //std::chrono::time_point<std::chrono::steady_clock> start{};
        //std::chrono::time_point<std::chrono::steady_clock> end{};
        FunctionTimerInterval start{};
        FunctionTimerInterval end{};

        auto calculate_duration() const -> FunctionTimerIntervalUnderlyingType
        {
            return FunctionTimerInterval{end - start}.count();
        }
    };

    class RC_FNCTMR_API FunctionTimerFrame
    {
    public:
        std::string function_name{};
        size_t self_frame_index{};
        size_t end_frame_index{};
        size_t hits{1};
        std::vector<FunctionTimerInternalFrame> hit_frames{};

    public:
        FunctionTimerFrame(std::string function_name) : self_frame_index(s_frame_stack.size()), end_frame_index(s_frame_stack.size()), function_name(function_name)
        {
            hit_frames.reserve(100000);
        }

    public:
        static std::vector<FunctionTimerFrame> s_frame_stack;
        static std::unordered_map<std::string, size_t> s_functions_on_stack;
        static bool s_timer_enabled;

    public:
        static auto start_profiling() -> void
        {
            s_frame_stack.clear();
            s_frame_stack.reserve(100);
            s_functions_on_stack.clear();
            s_timer_enabled = true;
        }

        static auto stop_profiling() -> void
        {
            s_timer_enabled = false;
        }

        static auto new_frame(const std::string& function_name) -> FunctionTimerFrame*
        {
            FunctionTimerFrame* frame{};

            auto it = s_functions_on_stack.find(function_name);
            if (it == s_functions_on_stack.end())
            {
                frame = &FunctionTimerFrame::s_frame_stack.emplace_back(function_name);
                s_functions_on_stack.emplace(function_name, frame->self_frame_index);
            }
            else
            {
                frame = &FunctionTimerFrame::s_frame_stack[it->second];
            }

            frame->hits++;
            return frame;
        }

        static auto dump_profile() -> void
        {
            Output::Targets<Output::NewFileDevice> scoped_out;
            auto& file_device = scoped_out.get_device<Output::NewFileDevice>();
            file_device.set_file_name_and_path(STR("UE4SS_Internal_Profiler_Results.txt"));

            scoped_out.send(STR("Duration is in seconds\n"));
            scoped_out.send(STR("Scientific notation is likely a tiny number\n\n\n"));

            //std::vector<int> vec{10, 5, 8, 3, 2, 8};

            for (const auto& frame : s_frame_stack)
            {
                auto inclusive_duration = frame.calculate_duration();
                auto exclusive_duration = inclusive_duration;

                std::vector<FunctionTimerIntervalUnderlyingType> average_exclusive_vec{};
                for (size_t i = frame.self_frame_index + 1; i < frame.end_frame_index; i++)
                {
                    const auto& called_frame = s_frame_stack[i];
                    //const auto dur = called_frame.calculate_duration();
                    //exclusive_duration -= dur;

                    for (const auto& hit_frame : called_frame.hit_frames)
                    {
                        const auto dur = hit_frame.calculate_duration();
                        exclusive_duration -= dur;
                        average_exclusive_vec.emplace_back(dur);
                    }
                }

                std::vector<FunctionTimerIntervalUnderlyingType> average_inclusive_vec{};
                for (const auto& hit_frame : frame.hit_frames)
                {
                    average_inclusive_vec.emplace_back(hit_frame.calculate_duration());
                }

                //auto average_inclusive = std::accumulate(average_inclusive_vec.begin(), average_inclusive_vec.end(), 0ll, [n = 0](auto cma, auto i) mutable {
                //    return cma + (i - cma) / ++n;
                //});
                //
                //auto average_exclusive = std::accumulate(average_exclusive_vec.begin(), average_exclusive_vec.end(), 0ll, [n = 0](auto cma, auto i) mutable {
                //    return cma + (i - cma) / ++n;
                //});

                FunctionTimerIntervalUnderlyingType average_inclusive{};
                if (!frame.hit_frames.empty())
                {
                    average_inclusive = inclusive_duration / frame.hit_frames.size();
                }
                else
                {
                    Output::send(STR("No hit_frames\n"));
                }

                FunctionTimerIntervalUnderlyingType average_exclusive{};
                if (!average_exclusive_vec.empty())
                {
                    average_exclusive = exclusive_duration / average_exclusive_vec.size();
                }
                else
                {
                    Output::send(STR("No frames captured between the start and end of this frame\n"));
                }

                //if (average_exclusive == 0) { average_exclusive = average_exclusive_vec.empty() ? exclusive_duration : average_exclusive_vec.front();
                if (average_exclusive == 0) { average_exclusive = average_inclusive; }
                //if (average_inclusive == 0) { average_inclusive = average_inclusive_vec.empty() ? average_exclusive : average_inclusive_vec.front(); }

                scoped_out.send(STR("Frame: {}\n"), to_wstring(frame.function_name));
                scoped_out.send(STR("\tHits: {}\n"), frame.hits);
                scoped_out.send(STR("\tDuration (ns) <only-function>: {} (avg: {})\n"), exclusive_duration, average_exclusive);
                scoped_out.send(STR("\tDuration (ns) <inclusive>    : {} (avg: {})\n\n"), inclusive_duration, average_inclusive);
            }
        }

    public:
        auto start_timer() -> void
        {
            hit_frames.emplace_back().start = std::chrono::duration_cast<FunctionTimerInterval>(FunctionTimerClockType::now().time_since_epoch());
        }

        auto stop_timer() -> void
        {
            auto& hit_frame = hit_frames.front();
            hit_frame.end = std::chrono::duration_cast<FunctionTimerInterval>(FunctionTimerClockType::now().time_since_epoch());
            end_frame_index = s_frame_stack.size() - 1;

            if (hit_frame.end.count() == 0)
            {
                abort();
            }
        }

        auto calculate_duration() const -> FunctionTimerIntervalUnderlyingType
        {
            FunctionTimerIntervalUnderlyingType duration{};
            for (const auto& hit_frame : hit_frames)
            {
                if (hit_frame.end < hit_frame.start) { continue; }
                duration += hit_frame.calculate_duration();
            }
            return duration;
        }
    };

    class RC_FNCTMR_API FunctionTimerFrameGuard
    {
    public:
        size_t self_frame_index{};

    public:
        ~FunctionTimerFrameGuard()
        {
            if (!FunctionTimerFrame::s_frame_stack.empty() && self_frame_index != -1)
            {
                FunctionTimerFrame::s_frame_stack[self_frame_index].stop_timer();
            }
        }
    };

#if defined(TIME_FUNCTION_MACRO_ENABLED)
#ifndef TIME_FUNCTION
#define TIME_FUNCTION() \
FunctionTimerFrame* function_timer_frame{}; \
if (FunctionTimerFrame::s_timer_enabled) \
{ \
    function_timer_frame = FunctionTimerFrame::new_frame(__FUNCSIG__); \
    function_timer_frame->start_timer();  \
} \
FunctionTimerFrameGuard function_timer_frame_guard{function_timer_frame ? function_timer_frame->self_frame_index : -1};
#endif
#else
#define TIME_FUNCTION()
#endif

#endif

}

#endif //RC_FUNCTION_TIMER_HPP

#ifdef __linux__

#include <CrashDumper.hpp>

#include <array>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include <execinfo.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

namespace RC
{
    namespace
    {
        constexpr std::array<int, 5> handled_signals{SIGSEGV, SIGABRT, SIGILL, SIGFPE, SIGBUS};
        std::array<struct sigaction, handled_signals.size()> previous_actions{};
        std::array<bool, handled_signals.size()> action_installed{};
        int crash_log_fd{-1};
        volatile sig_atomic_t handling_signal{};

        auto write_all(int file_descriptor, const char* data, size_t size) -> void
        {
            while (size > 0)
            {
                const auto written = write(file_descriptor, data, size);
                if (written < 0)
                {
                    if (errno == EINTR)
                    {
                        continue;
                    }
                    return;
                }
                data += written;
                size -= static_cast<size_t>(written);
            }
        }

        auto write_signal_number(int file_descriptor, int signal_number) -> void
        {
            constexpr char prefix[] = "UE4SS caught signal ";
            write_all(file_descriptor, prefix, sizeof(prefix) - 1);

            char digits[16]{};
            size_t digit_count{};
            auto value = static_cast<unsigned int>(signal_number);
            do
            {
                digits[digit_count++] = static_cast<char>('0' + value % 10);
                value /= 10;
            } while (value > 0 && digit_count < sizeof(digits));

            while (digit_count > 0)
            {
                --digit_count;
                write_all(file_descriptor, &digits[digit_count], 1);
            }
            write_all(file_descriptor, "\nBacktrace:\n", 12);
        }

        auto signal_index(int signal_number) -> size_t
        {
            for (size_t index = 0; index < handled_signals.size(); ++index)
            {
                if (handled_signals[index] == signal_number)
                {
                    return index;
                }
            }
            return handled_signals.size();
        }

        auto crash_signal_handler(int signal_number) -> void
        {
            if (handling_signal != 0)
            {
                _exit(128 + signal_number);
            }
            handling_signal = 1;

            const auto output_fd = crash_log_fd >= 0 ? crash_log_fd : STDERR_FILENO;
            write_signal_number(output_fd, signal_number);

            std::array<void*, 64> frames{};
            const auto frame_count = backtrace(frames.data(), static_cast<int>(frames.size()));
            backtrace_symbols_fd(frames.data(), frame_count, output_fd);
            write_all(output_fd, "\n", 1);

            const auto index = signal_index(signal_number);
            if (index < handled_signals.size() && action_installed[index])
            {
                sigaction(signal_number, &previous_actions[index], nullptr);
            }
            kill(getpid(), signal_number);
            _exit(128 + signal_number);
        }
    } // namespace

    CrashDumper::CrashDumper() = default;

    CrashDumper::~CrashDumper()
    {
        if (enabled)
        {
            for (size_t index = 0; index < handled_signals.size(); ++index)
            {
                if (action_installed[index])
                {
                    sigaction(handled_signals[index], &previous_actions[index], nullptr);
                    action_installed[index] = false;
                }
            }
        }
        if (crash_log_fd >= 0)
        {
            close(crash_log_fd);
            crash_log_fd = -1;
        }
        enabled = false;
    }

    void CrashDumper::enable()
    {
        if (enabled)
        {
            return;
        }

        const auto* configured_directory = std::getenv("UE4SS_CRASH_LOG_DIR");
        std::filesystem::path output_directory = configured_directory && configured_directory[0] != '\0' ? configured_directory : ".";
        std::error_code error{};
        std::filesystem::create_directories(output_directory, error);
        if (error)
        {
            std::fprintf(stderr, "UE4SS: failed to create crash-log directory: %s\n", error.message().c_str());
            return;
        }

        const auto output_path = output_directory / ("crash_" + std::to_string(getpid()) + ".log");
        crash_log_fd = open(output_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, 0644);
        if (crash_log_fd < 0)
        {
            std::fprintf(stderr, "UE4SS: failed to open crash log '%s': %s\n", output_path.c_str(), std::strerror(errno));
            return;
        }

        // Prime the unwinder before entering an async signal context.
        void* warmup_frame{};
        backtrace(&warmup_frame, 1);

        struct sigaction action{};
        sigemptyset(&action.sa_mask);
        action.sa_handler = &crash_signal_handler;
        action.sa_flags = SA_RESETHAND;

        for (size_t index = 0; index < handled_signals.size(); ++index)
        {
            if (sigaction(handled_signals[index], &action, &previous_actions[index]) == 0)
            {
                action_installed[index] = true;
            }
            else
            {
                std::fprintf(stderr, "UE4SS: failed to install handler for signal %d: %s\n", handled_signals[index], std::strerror(errno));
            }
        }
        handling_signal = 0;
        enabled = true;
    }

    void CrashDumper::set_full_memory_dump(bool full_memory_dump_enabled)
    {
        if (full_memory_dump_enabled)
        {
            std::fprintf(stderr, "UE4SS: full-memory minidumps are unavailable on Linux; signal backtraces remain enabled.\n");
        }
    }
} // namespace RC

#endif // __linux__

#include "status.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
    std::mutex g_log_mutex;

    [[nodiscard]] std::string json_escape(std::string_view input)
    {
        std::ostringstream output;
        for (const unsigned char character : input)
        {
            switch (character)
            {
            case '"':
                output << "\\\"";
                break;
            case '\\':
                output << "\\\\";
                break;
            case '\b':
                output << "\\b";
                break;
            case '\f':
                output << "\\f";
                break;
            case '\n':
                output << "\\n";
                break;
            case '\r':
                output << "\\r";
                break;
            case '\t':
                output << "\\t";
                break;
            default:
                if (character < 0x20u)
                {
                    output << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<unsigned int>(character) << std::dec;
                }
                else
                {
                    output << character;
                }
            }
        }
        return output.str();
    }

    [[nodiscard]] const char* state_name(ue4ss_runtime_state state)
    {
        switch (state)
        {
        case UE4SS_STATE_NOT_STARTED:
            return "not_started";
        case UE4SS_STATE_INITIALIZING:
            return "initializing";
        case UE4SS_STATE_PLATFORM_READY:
            return "platform_ready";
        case UE4SS_STATE_DEGRADED:
            return "degraded";
        case UE4SS_STATE_FAILED:
            return "failed";
        case UE4SS_STATE_STOPPED:
            return "stopped";
        }
        return "unknown";
    }

    [[nodiscard]] const char* capability_name(ue4ss::linux::core::CapabilityState state)
    {
        using ue4ss::linux::core::CapabilityState;
        switch (state)
        {
        case CapabilityState::available:
            return "available";
        case CapabilityState::unavailable:
            return "unavailable";
        case CapabilityState::disabled:
            return "disabled";
        }
        return "unknown";
    }

    void write_all(int descriptor, std::string_view contents)
    {
        std::size_t written = 0;
        while (written < contents.size())
        {
            const ssize_t result = write(descriptor, contents.data() + written, contents.size() - written);
            if (result > 0)
            {
                written += static_cast<std::size_t>(result);
                continue;
            }
            if (result < 0 && errno == EINTR)
            {
                continue;
            }
            throw std::runtime_error(std::string{"write: "} + std::strerror(errno));
        }
    }

    template <std::size_t Size>
    void copy_string(char (&destination)[Size], const std::string& source)
    {
        const std::size_t amount = std::min(source.size(), Size - 1u);
        std::memcpy(destination, source.data(), amount);
        destination[amount] = '\0';
    }

    [[nodiscard]] std::string status_json(const ue4ss::linux::core::RuntimeData& runtime)
    {
        std::ostringstream output;
        output << "{\n"
               << "  \"schema_version\": 1,\n"
               << "  \"abi_version\": " << UE4SS_LINUX_ABI_VERSION << ",\n"
               << "  \"state\": \"" << state_name(runtime.state) << "\",\n"
               << "  \"message\": \"" << json_escape(runtime.message) << "\",\n"
               << "  \"required\": " << (runtime.required ? "true" : "false") << ",\n"
               << "  \"process_id\": " << getpid() << ",\n"
               << "  \"build\": {\"git_sha\": \"" << UE4SS_LINUX_GIT_SHA << "\", \"phase\": \"lua-hooks-mvp\"},\n"
               << "  \"executable\": {\n"
               << "    \"path\": \"" << json_escape(runtime.executable_path) << "\",\n"
               << "    \"sha256\": \"" << runtime.image.sha256 << "\",\n"
               << "    \"build_id\": \"" << runtime.image.build_id << "\",\n"
               << "    \"elf_class\": " << static_cast<unsigned int>(runtime.image.elf_class) << ",\n"
               << "    \"machine\": " << runtime.image.machine << "\n"
               << "  },\n"
               << "  \"module_count\": " << runtime.image.modules.size() << ",\n"
               << "  \"capabilities\": {\n";
        for (std::size_t index = 0; index < runtime.capabilities.size(); ++index)
        {
            const auto& capability = runtime.capabilities[index];
            output << "    \"" << json_escape(capability.name) << "\": {\"state\": \"" << capability_name(capability.state) << "\", \"detail\": \""
                   << json_escape(capability.detail) << "\"}";
            output << (index + 1u == runtime.capabilities.size() ? "\n" : ",\n");
        }
        output << "  }\n}\n";
        return output.str();
    }
} // namespace

namespace ue4ss::linux::core
{
    void log_line(const RuntimeData& runtime, const std::string& level, const std::string& message)
    {
        std::lock_guard lock{g_log_mutex};
        const auto now = std::chrono::system_clock::now();
        const auto seconds = std::chrono::system_clock::to_time_t(now);
        std::tm local_time{};
        localtime_r(&seconds, &local_time);
        std::ostringstream line;
        line << '[' << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S") << "] [" << level << "] " << message << '\n';
        const std::string text = line.str();
        try
        {
            write_all(STDERR_FILENO, text);
        }
        catch (...)
        {
        }

        if (!runtime.state_path.empty())
        {
            const std::filesystem::path log_path = std::filesystem::path{runtime.state_path} / "ue4ss.log";
            const int descriptor = open(log_path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
            if (descriptor >= 0)
            {
                try
                {
                    write_all(descriptor, text);
                }
                catch (...)
                {
                }
                close(descriptor);
            }
        }
    }

    void write_status_file(const RuntimeData& runtime)
    {
        if (runtime.state_path.empty())
        {
            return;
        }
        std::filesystem::create_directories(runtime.state_path);
        const std::filesystem::path final_path = std::filesystem::path{runtime.state_path} / "ue4ss.status.json";
        const std::filesystem::path temporary_path = final_path.string() + ".tmp." + std::to_string(getpid());
        const int descriptor = open(temporary_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
        if (descriptor < 0)
        {
            throw std::runtime_error(std::string{"open("} + temporary_path.string() + "): " + std::strerror(errno));
        }
        try
        {
            write_all(descriptor, status_json(runtime));
            if (fsync(descriptor) != 0)
            {
                throw std::runtime_error(std::string{"fsync("} + temporary_path.string() + "): " + std::strerror(errno));
            }
        }
        catch (...)
        {
            close(descriptor);
            unlink(temporary_path.c_str());
            throw;
        }
        if (close(descriptor) != 0)
        {
            unlink(temporary_path.c_str());
            throw std::runtime_error(std::string{"close("} + temporary_path.string() + "): " + std::strerror(errno));
        }
        if (rename(temporary_path.c_str(), final_path.c_str()) != 0)
        {
            unlink(temporary_path.c_str());
            throw std::runtime_error(std::string{"rename("} + final_path.string() + "): " + std::strerror(errno));
        }
    }

    void copy_public_status(const RuntimeData& runtime, ue4ss_status& status)
    {
        std::memset(&status, 0, sizeof(status));
        status.struct_size = sizeof(status);
        status.abi_version = UE4SS_LINUX_ABI_VERSION;
        status.state = runtime.state;
        status.capability_count = static_cast<std::uint32_t>(runtime.capabilities.size());
        status.last_error = runtime.last_error;
        status.process_id = static_cast<std::uint32_t>(getpid());
        copy_string(status.message, runtime.message);
        copy_string(status.executable_path, runtime.executable_path);
        copy_string(status.state_path, runtime.state_path);
        copy_string(status.executable_sha256, runtime.image.sha256);
        copy_string(status.executable_build_id, runtime.image.build_id);
    }
} // namespace ue4ss::linux::core

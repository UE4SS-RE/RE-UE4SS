#pragma once

#include <filesystem>
#include <string>

namespace RC::Platform
{
    class SharedLibrary
    {
      private:
        void* m_handle{};
        void* m_search_path_cookie{};
        std::string m_error{};

      public:
        SharedLibrary() = default;
        explicit SharedLibrary(const std::filesystem::path& path);
        ~SharedLibrary();
        SharedLibrary(const SharedLibrary&) = delete;
        auto operator=(const SharedLibrary&) -> SharedLibrary& = delete;
        SharedLibrary(SharedLibrary&& other) noexcept;
        auto operator=(SharedLibrary&& other) noexcept -> SharedLibrary&;

        auto open(const std::filesystem::path& path) -> bool;
        auto resolve(const char* symbol_name) -> void*;
        auto close() -> void;
        [[nodiscard]] auto is_open() const -> bool;
        [[nodiscard]] auto error() const -> const std::string&;
    };
} // namespace RC::Platform

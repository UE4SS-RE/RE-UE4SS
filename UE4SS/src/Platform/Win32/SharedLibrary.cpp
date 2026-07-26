#ifdef _WIN32

#include <Platform/SharedLibrary.hpp>

#include <utility>

#define NOMINMAX
#include <Windows.h>

#include <Helpers/SysError.hpp>

namespace RC::Platform
{
    SharedLibrary::SharedLibrary(const std::filesystem::path& path)
    {
        open(path);
    }

    SharedLibrary::~SharedLibrary()
    {
        close();
    }

    SharedLibrary::SharedLibrary(SharedLibrary&& other) noexcept
        : m_handle{std::exchange(other.m_handle, nullptr)}, m_search_path_cookie{std::exchange(other.m_search_path_cookie, nullptr)},
          m_error{std::move(other.m_error)}
    {
    }

    auto SharedLibrary::operator=(SharedLibrary&& other) noexcept -> SharedLibrary&
    {
        if (this != &other)
        {
            close();
            m_handle = std::exchange(other.m_handle, nullptr);
            m_search_path_cookie = std::exchange(other.m_search_path_cookie, nullptr);
            m_error = std::move(other.m_error);
        }
        return *this;
    }

    auto SharedLibrary::open(const std::filesystem::path& path) -> bool
    {
        close();
        m_search_path_cookie = AddDllDirectory(path.parent_path().c_str());
        m_handle = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!m_handle)
        {
            m_error = SysError(GetLastError()).c_str();
            if (m_search_path_cookie)
            {
                RemoveDllDirectory(m_search_path_cookie);
                m_search_path_cookie = nullptr;
            }
            return false;
        }
        m_error.clear();
        return true;
    }

    auto SharedLibrary::resolve(const char* symbol_name) -> void*
    {
        if (!m_handle)
        {
            m_error = "Cannot resolve a symbol from a closed shared library";
            return nullptr;
        }
        auto* symbol = reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(m_handle), symbol_name));
        if (!symbol)
        {
            m_error = SysError(GetLastError()).c_str();
            return nullptr;
        }
        m_error.clear();
        return symbol;
    }

    auto SharedLibrary::close() -> void
    {
        if (m_handle)
        {
            FreeLibrary(static_cast<HMODULE>(m_handle));
            m_handle = nullptr;
        }
        if (m_search_path_cookie)
        {
            RemoveDllDirectory(m_search_path_cookie);
            m_search_path_cookie = nullptr;
        }
    }

    auto SharedLibrary::is_open() const -> bool
    {
        return m_handle != nullptr;
    }

    auto SharedLibrary::error() const -> const std::string&
    {
        return m_error;
    }
} // namespace RC::Platform

#endif // _WIN32

#ifdef __linux__

#include <Platform/SharedLibrary.hpp>

#include <utility>

#include <dlfcn.h>

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
        dlerror();
        m_handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!m_handle)
        {
            if (const auto* error_message = dlerror())
            {
                m_error = error_message;
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

        dlerror();
        auto* symbol = dlsym(m_handle, symbol_name);
        if (const auto* error_message = dlerror())
        {
            m_error = error_message;
            return nullptr;
        }
        m_error.clear();
        return symbol;
    }

    auto SharedLibrary::close() -> void
    {
        if (m_handle)
        {
            if (dlclose(m_handle) != 0)
            {
                if (const auto* error_message = dlerror())
                {
                    m_error = error_message;
                }
            }
            m_handle = nullptr;
        }
        m_search_path_cookie = nullptr;
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

#endif // __linux__

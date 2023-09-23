#ifndef UE4SS_CANERROR_HPP
#define UE4SS_CANERROR_HPP

// Define MPROGRAM_USE_EXCEPTIONS to enable exceptions

#include <memory>
#include <stdexcept>
#include <utility>

#include <ErrorObject.hpp>

namespace RC
{
    class CanError
    {
    protected:
        std::shared_ptr<ErrorObject> m_error_object{};

    public:
        CanError() = default;

        CanError(std::shared_ptr<ErrorObject> error_object) : m_error_object(std::move(error_object)) {};

    protected:
        auto set_error(const char* message) -> void
        {
            m_error_object->copy_error(message);
            m_error_object->m_has_error = true;

#ifdef MPROGRAM_USE_EXCEPTIONS
            throw std::runtime_error{m_error_object->get_message()};
#endif
        }

        auto set_error(const char* message, bool program_should_close) -> void
        {
            m_error_object->copy_error(message);
            m_error_object->m_close_program = program_should_close;
            m_error_object->m_has_error = true;

#ifdef MPROGRAM_USE_EXCEPTIONS
            throw std::runtime_error{m_error_object->get_message()};
#endif
        }

        template<typename ...Args>
        auto set_error(const char* fmt, Args... args) -> void
        {
            m_error_object->copy_error(fmt, args...);
            m_error_object->m_has_error = true;

#ifdef MPROGRAM_USE_EXCEPTIONS
            throw std::runtime_error{m_error_object->get_message()};
#endif
        }

        auto copy_error_into_message(const char* message)
        {
            m_error_object->copy_error(message);
            m_error_object->m_has_error = true;
        }

        template <typename ...Args>
        auto copy_error_into_message(const char* fmt, Args... args) noexcept -> void
        {
            m_error_object->copy_error(fmt, args...);
            m_error_object->m_has_error = true;
        }

    public:
        auto get_error_object() noexcept -> std::shared_ptr<ErrorObject>
        {
            return m_error_object;
        }
    };
}

#endif //UE4SS_CANERROR_HPP

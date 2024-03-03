#pragma once

#include <cstdio>
#include <cstring>

#ifdef LINUX
#define sprintf_s snprintf
#define strncpy_s(dest, destsz, src, count) strncpy((dest), (src), (((count) < (destsz)) ? (count) : (destsz)))
#endif

namespace RC
{
    class ErrorObject
    {
      private:
        static constexpr unsigned long long m_message_buffer_max_size = 2000;

        char m_message[m_message_buffer_max_size]{"An error occurred but no error message was supplied."};
        bool m_close_program{false};
        bool m_has_error{false};

        // Allow the 'CanError' class to modify the contents of the error object
        friend class CanError;

      public:
        ErrorObject(bool default_should_close_program = false) noexcept
        {
            m_close_program = default_should_close_program;
        }

        ErrorObject(const char* error_message) noexcept
        {
            copy_error(error_message);
        }

        ErrorObject(const char* error_message, bool should_program_close) noexcept
        {
            copy_error(error_message);
            m_close_program = should_program_close;
        }

      protected:
        auto copy_error(const char* message) noexcept -> void
        {
            size_t msg_len = strlen(message);

            // Attempt to give a hint if the buffer is too small
            if (msg_len > sizeof(m_message))
            {
                message = "An error occurred but the message was too long for the buffer.";
                msg_len = strlen(message);
            }

            // If the buffer is too small for the hint message then I guess we do nothing
            // The default message will be used which can't be too small since it's calculated at compile-time
            if (msg_len < sizeof(m_message))
            {
                strncpy_s(m_message, msg_len, message, msg_len);
            }
        }

        template <typename... Args>
        auto copy_error(const char* fmt, Args... args) noexcept -> void
        {
            size_t msg_len = strlen(fmt);

            // Attempt to give a hint if the buffer is too small
            if (msg_len > sizeof(m_message))
            {
                fmt = "An error occurred but the message was too long for the buffer.";
                msg_len = strlen(fmt);
            }

            // If the buffer is too small for the hint message then I guess we do nothing
            // The default message will be used which can't be too small since it's calculated at compile-time
            if (msg_len < sizeof(m_message))
            {
                sprintf_s(m_message, sizeof(m_message), fmt, args...);
            }
        }

      public:
        [[nodiscard]] auto get_message() noexcept -> const char*
        {
            return m_message;
        }

        [[nodiscard]] auto program_should_close() noexcept -> bool
        {
            return m_close_program;
        }

        [[nodiscard]] auto has_error() noexcept -> bool
        {
            return m_has_error;
        }
    };
} // namespace RC

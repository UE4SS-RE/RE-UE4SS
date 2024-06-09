#pragma once

#include <Input/Platform/QueueInputSource.hpp>
#include <vector>

namespace RC::Input
{

    class NcursesInputSource : public QueueInputSource
    {
      public:
        auto begin_frame() -> void;
        auto receive_input(int input) -> void;
        auto end_frame() -> void;

      private:
        /// SAFETY: Only update inside begin_frame ... receive_input(...) ... end_frame
        std::vector<bool> m_key_cur{};
        std::vector<bool> m_key_last{};
        bool m_is_alt_down;

      public:
        NcursesInputSource() : m_is_alt_down(false)
        {
            m_key_cur.resize(256, false);
            m_key_last.resize(256, false);
        }

        auto is_available() -> bool override;
        const char* get_name() override
        {
            return "Ncurses";
        }
    };

} // namespace RC::Input

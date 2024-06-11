#pragma once

#include <Input/Platform/QueueInputSource.hpp>
#include <vector>

namespace RC::Input
{

    class GLFW3InputSource : public QueueInputSource
    {
      public:
        auto begin_frame() -> void;
        auto receive_input(int key, int action, int mods) -> void;
        auto end_frame() -> void;

      private:
        Key m_translate_key[512];

      public:
        auto is_available() -> bool override;
        const char* get_name() override
        {
            return "GLFW3";
        }
        GLFW3InputSource();
    };

} // namespace RC::Input

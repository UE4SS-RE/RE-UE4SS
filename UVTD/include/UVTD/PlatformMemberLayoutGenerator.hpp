#pragma once

#include <filesystem>
#include <vector>

#include <UVTD/Config.hpp>

namespace RC::UVTD
{
    class PlatformMemberLayoutGenerator
    {
      private:
        std::vector<PlatformMemberLayout> m_layouts;
        std::filesystem::path m_output_root;

      public:
        PlatformMemberLayoutGenerator(
            std::vector<PlatformMemberLayout> layouts,
            std::filesystem::path output_root);

        auto generate_files() const -> void;
        auto output_cleanup() const -> void;
    };
}

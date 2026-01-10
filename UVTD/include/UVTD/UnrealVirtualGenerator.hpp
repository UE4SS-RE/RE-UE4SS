#pragma once

#include <UVTD/PDBNameInfo.hpp>
#include <UVTD/TypeContainer.hpp>

namespace RC::UVTD
{
    class UnrealVirtualGenerator
    {
      private:
        PDBNameInfo pdb_info;
        TypeContainer type_container;

      public:
        UnrealVirtualGenerator() = delete;

        explicit UnrealVirtualGenerator(const PDBNameInfo& pdb_info, TypeContainer container)
            : pdb_info(pdb_info), type_container(std::move(container))
        {
        }

      public:
        auto generate_files() -> void;

      public:
        static auto output_cleanup() -> void;
    };
} // namespace RC::UVTD
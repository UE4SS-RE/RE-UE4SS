#ifndef UNREALVTABLEDUMPER_UNREALVIRTUALGENERATOR_HPP
#define UNREALVTABLEDUMPER_UNREALVIRTUALGENERATOR_HPP

#include <UVTD/TypeContainer.hpp>

namespace RC::UVTD
{
    class UnrealVirtualGenerator
    {
      private:
        SystemStringType pdb_name;
        TypeContainer type_container;

      public:
        UnrealVirtualGenerator() = delete;

        explicit UnrealVirtualGenerator(SystemStringType pdb_name, TypeContainer container)
            : pdb_name(std::move(pdb_name)), type_container(std::move(container))
        {
        }

      public:
        auto generate_files() -> void;

      public:
        static auto output_cleanup() -> void;
    };
} // namespace RC::UVTD

#endif
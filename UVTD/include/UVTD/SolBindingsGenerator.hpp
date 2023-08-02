#ifndef UNREALVTABLEDUMPER_SOLBINDINGSGENERATOR_HPP
#define UNREALVTABLEDUMPER_SOLBINDINGSGENERATOR_HPP

#include <unordered_map>

#include <File/File.hpp>
#include <UVTD/Symbols.hpp>
#include <UVTD/TypeContainer.hpp>

namespace RC::UVTD
{
    class SolBindingsGenerator
    {
      private:
        Symbols symbols;
        TypeContainer type_container;

      public:
        SolBindingsGenerator() = delete;

        explicit SolBindingsGenerator(Symbols symbols) : symbols(std::move(symbols))
        {
        }

      public:
        auto generate_code() -> void;
        auto generate_files() -> void;

      public:
        static auto output_cleanup() -> void;
    };
} // namespace RC::UVTD

#endif
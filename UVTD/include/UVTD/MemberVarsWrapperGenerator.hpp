#ifndef UNREALVTABLEDUMPER_MEMBERVARSWRAPPERGENERATOR_HPP
#define UNREALVTABLEDUMPER_MEMBERVARSWRAPPERGENERATOR_HPP

#include <UVTD/TypeContainer.hpp>

namespace RC::UVTD
{
    class MemberVarsWrapperGenerator
    {
      private:
        TypeContainer type_container;

      public:
        MemberVarsWrapperGenerator() = delete;

        explicit MemberVarsWrapperGenerator(TypeContainer container) : type_container(std::move(container))
        {
        }

      public:
        auto generate_files() -> void;

      public:
        static auto output_cleanup() -> void;
    };
} // namespace RC::UVTD

#endif
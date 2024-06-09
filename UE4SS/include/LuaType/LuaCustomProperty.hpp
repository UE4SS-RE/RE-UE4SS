#pragma once

#include <memory>
#include <string>
#include <vector>

#include <File/Macros.hpp>

namespace RC::Unreal
{
    class UObject;
    class UClass;
    class FProperty;
    class CustomProperty;
} // namespace RC::Unreal

namespace RC::LuaType
{
    class LuaCustomProperty
    {
      public:
        SystemStringType m_name{};
        std::unique_ptr<Unreal::CustomProperty> m_property;

      public:
        LuaCustomProperty(SystemStringType name, std::unique_ptr<Unreal::CustomProperty> property);

      private:
        class PropertyList
        {
          private:
            std::vector<LuaCustomProperty> properties;

          public:
            auto add(SystemStringType property_name, std::unique_ptr<Unreal::CustomProperty>) -> void;
            auto clear() -> void;
            auto find_or_nullptr(Unreal::UObject* base, SystemStringType property_name) -> Unreal::FProperty*;
        };

      public:
        struct StaticStorage
        {
            static PropertyList property_list;
        };
    };
} // namespace RC::LuaType

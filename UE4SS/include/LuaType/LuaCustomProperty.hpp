#pragma once

#include <memory>
#include <string>
#include <vector>

#include <String/StringType.hpp>

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
        StringType m_name{};
        std::unique_ptr<Unreal::CustomProperty> m_property;

      public:
        LuaCustomProperty(StringType name, std::unique_ptr<Unreal::CustomProperty> property);

      private:
        class PropertyList
        {
          private:
            std::vector<LuaCustomProperty> properties;

          public:
            auto add(StringType property_name, std::unique_ptr<Unreal::CustomProperty>) -> void;
            auto clear() -> void;
            auto find_or_nullptr(Unreal::UObject* base, StringType property_name) -> Unreal::FProperty*;
        };

      public:
        struct StaticStorage
        {
            static PropertyList property_list;
        };
    };
} // namespace RC::LuaType

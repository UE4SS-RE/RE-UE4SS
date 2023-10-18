#pragma once

#include <LuaType/LuaUObject.hpp>

namespace RC::Unreal
{
    class FOutputDevice;
}

namespace RC::LuaType
{
    struct FOutputDeviceName
    {
        constexpr static const char* ToString()
        {
            return "FOutputDevice";
        }
    };
    class FOutputDevice : public RemoteObjectBase<Unreal::FOutputDevice, FOutputDeviceName>
    {
      private:
        explicit FOutputDevice(Unreal::FOutputDevice* object);

      public:
        FOutputDevice() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FOutputDevice*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType

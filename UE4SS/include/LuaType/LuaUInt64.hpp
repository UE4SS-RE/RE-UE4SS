#pragma once

#include <cstdint>

#include <LuaType/LuaUObject.hpp>

namespace RC::LuaType
{
    struct UInt64Name
    {
        constexpr static const char* ToString()
        {
            return "UInt64";
        }
    };
    class UInt64 : public LocalObjectBase<uint64_t, UInt64Name>
    {
      private:
        explicit UInt64(uint64_t object);

      public:
        UInt64() = delete;
        auto static construct(const LuaMadeSimple::Lua&, uint64_t) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;
    };
} // namespace RC::LuaType

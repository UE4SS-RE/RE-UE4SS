#pragma once

#include <thread>

#include <LuaType/LuaUObject.hpp>

namespace RC::LuaType
{
    struct ThreadIdName
    {
        constexpr static const char* ToString()
        {
            return "ThreadId";
        }
    };
    class ThreadId : public LocalObjectBase<std::thread::id, ThreadIdName>
    {
      private:
        explicit ThreadId(std::thread::id object);

      public:
        ThreadId() = delete;
        auto static construct(const LuaMadeSimple::Lua&, std::thread::id) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

    private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType

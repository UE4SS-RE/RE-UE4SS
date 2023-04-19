#ifndef UE4SS_REWRITTEN_CPPMODUSERBASE_HPP
#define UE4SS_REWRITTEN_CPPMODUSERBASE_HPP

#include <Common.hpp>

namespace RC
{
    // When making C++ mods, keep in mind that they will break if UE4SS and the mod don't use the same C Runtime library version
    // This includes them being compiled in different configurations (Debug/Release).
    class RC_UE4SS_API CppUserModBase
    {
    public:
        CppUserModBase() = default;
        virtual ~CppUserModBase() = default;

    public:
        auto virtual update() -> void = 0;
    };
}

#endif // UE4SS_REWRITTEN_CPPMODUSERBASE_HPP
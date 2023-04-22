#ifndef UE4SS_REWRITTEN_CPPMODUSERBASE_HPP
#define UE4SS_REWRITTEN_CPPMODUSERBASE_HPP

#include <Common.hpp>
#include <File/Macros.hpp>

namespace RC
{
    struct ModMetadata
    {
        const StringType ModName{};
        const StringType ModVersion{};
        const StringType ModDescription{};
        const StringType ModAuthors{};
        const StringType ModIntendedSDKVersion{};
    };

    // When making C++ mods, keep in mind that they will break if UE4SS and the mod don't use the same C Runtime library version
    // This includes them being compiled in different configurations (Debug/Release).
    class RC_UE4SS_API CppUserModBase
    {
    public:
        StringType ModName{};
        StringType ModVersion{};
        StringType ModDescription{};
        StringType ModAuthors{};
        StringType ModIntendedSDKVersion{};

    public:
        CppUserModBase();
        virtual ~CppUserModBase() = default;

    public:
        auto virtual update() -> void = 0;
    };
}

#endif // UE4SS_REWRITTEN_CPPMODUSERBASE_HPP
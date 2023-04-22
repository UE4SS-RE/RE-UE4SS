#include <Mod/CppUserModBase.hpp>
#include <Mod/CppMod.hpp>

namespace RC
{
    CppUserModBase::CppUserModBase(const ModMetadata& Metadata) :
        ModName(Metadata.ModName),
        ModVersion(Metadata.ModVersion),
        ModDescription(Metadata.ModDescription),
        ModAuthors(Metadata.ModAuthors),
        ModIntendedSDKVersion(Metadata.ModIntendedSDKVersion)
    {
    }
}

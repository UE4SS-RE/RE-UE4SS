#define NOMINMAX

#include <memory>
#include <string>
#include <format>
#include <filesystem>
#include <stdexcept>
#include <limits>

#include <Mod/Mod.hpp>
#include <Mod/LuaMod.hpp>
#include <Helpers/Format.hpp>
#include <Helpers/String.hpp>
#include <ExceptionHandling.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#pragma warning(disable : 4005)
#include <UE4SSProgram.hpp>
#include <GUI/Dumpers.hpp>
#include <USMapGenerator/Generator.hpp>
#include <Unreal/UnrealVersion.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/UnrealVersion.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/FProperty.hpp>
#include <Unreal/UPackage.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UAssetRegistry.hpp>
#include <Unreal/UAssetRegistryHelpers.hpp>
#include <Unreal/UGameViewportClient.hpp>
#include <Unreal/PackageName.hpp>
#include <Unreal/TypeChecker.hpp>
#include <Unreal/FFrame.hpp>
#include <Unreal/FOutputDevice.hpp>
#include <Unreal/Property/NumericPropertyTypes.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/Property/FWeakObjectProperty.hpp>
#include <Unreal/Property/FClassProperty.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FMapProperty.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <Unreal/Property/FNameProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/Property/FTextProperty.hpp>
#include <Unreal/Property/FStrProperty.hpp>
#include <UnrealCustom/CustomProperty.hpp>
#include <Unreal/PrimitiveTypes.hpp>
#include <Unreal/UKismetSystemLibrary.hpp>
#include <Unreal/Hooks.hpp>
#pragma warning(default : 4005)

#include <Timer/FunctionTimer.hpp>

namespace RC
{

    Mod::Mod(UE4SSProgram& program, std::wstring&& mod_name, std::wstring&& mod_path) : m_program(program), m_mod_name(mod_name), m_mod_path(mod_path)
    {}

    auto Mod::get_name() const -> std::wstring_view
    {
        return m_mod_name;
    }

    auto Mod::set_installable(bool is_installable) -> void
    {
        m_installable = is_installable;
    }

    auto Mod::is_installable() const -> bool
    {
        return m_installable;
    }

    auto Mod::set_installed(bool is_installed) -> void
    {
        m_installed = is_installed;
    }

    auto Mod::is_installed() const -> bool
    {
        return m_installed;
    }

    auto Mod::is_started() const -> bool
    {
        return m_is_started;
    }
    
    auto Mod::fire_update() -> void {}

    auto Mod::update_async() -> void {}
}

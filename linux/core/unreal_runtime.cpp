#include "unreal_runtime.hpp"

#include "ue4ss/patternsleuth_abi.h"

#include <Unreal/FMemory.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UnrealVersion.hpp>

#include <bit>
#include <exception>

namespace ue4ss::linux::core
{
    bool apply_unreal_resolver_state(const ResolvedUnrealState& resolved, std::string& error) noexcept
    {
        constexpr std::uint64_t required = UE4SS_PS_GUOBJECT_ARRAY | UE4SS_PS_FNAME_TO_STRING | UE4SS_PS_FNAME_CTOR |
                                           UE4SS_PS_GMALLOC | UE4SS_PS_STATIC_CONSTRUCT_OBJECT | UE4SS_PS_GAME_ENGINE_TICK |
                                           UE4SS_PS_ENGINE_VERSION;
        if ((resolved.available_mask & required) != required)
        {
            error = "required resolver addresses are incomplete";
            return false;
        }
        if (resolved.engine_major < 4u || resolved.engine_major > 5u || resolved.engine_minor > 30u)
        {
            error = "resolved Unreal Engine version is outside the supported range";
            return false;
        }

        try
        {
            RC::Unreal::Version::Major = static_cast<std::int32_t>(resolved.engine_major);
            RC::Unreal::Version::Minor = static_cast<std::int32_t>(resolved.engine_minor);
            RC::Unreal::GUObjectArray = std::bit_cast<RC::Unreal::FUObjectArray*>(resolved.guobject_array);
            RC::Unreal::GMalloc = std::bit_cast<RC::Unreal::FMalloc**>(resolved.gmalloc);
            RC::Unreal::FName::ToStringInternal.assign_address(std::bit_cast<void*>(resolved.fname_to_string));
            RC::Unreal::FName::ConstructorInternal.assign_address(std::bit_cast<void*>(resolved.fname_ctor));
            RC::Unreal::UObjectGlobals::GlobalState::StaticConstructObjectInternal.assign_address(
                    std::bit_cast<void*>(resolved.static_construct_object));
            RC::Unreal::UObjectGlobals::GlobalState::StaticConstructObjectInternalDeprecated.assign_address(
                    std::bit_cast<void*>(resolved.static_construct_object));
            RC::Unreal::UEngine::TickInternal.assign_address(std::bit_cast<void*>(resolved.game_engine_tick));
            error.clear();
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while applying UEPseudo resolver state";
            return false;
        }
    }
}

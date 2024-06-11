#include <DynamicOutput/DynamicOutput.hpp>
#include <LuaLibrary.hpp>
#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include <SigScanner/SinglePassSigScanner.hpp>
#include <Signatures.hpp>
#include <Unreal/FMemory.hpp>
#include <Unreal/FString.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/Signatures.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UnrealInitializer.hpp>
#include <filesystem>

namespace RC
{
    auto scan_complete_default_func(DidLuaScanSucceed) -> void
    {
    }

    auto scan_from_lua_script(SystemStringType& script_file_path_and_name,
                              std::vector<SignatureContainer>& signature_containers,
                              LuaScriptMatchFoundFunc& match_found_func,
                              LuaScriptScanCompleteFunc& scan_complete_func) -> void
    {
        const LuaMadeSimple::Lua& lua = LuaMadeSimple::new_state();

        lua.open_all_libs();
        lua.register_function("Print", LuaLibrary::global_print);
        lua.register_function("print", LuaLibrary::global_print);
        lua.register_function("DerefToInt32", LuaLibrary::deref_to_int32);
        lua.register_function("dereftoint32", LuaLibrary::deref_to_int32);

        lua.execute_file(to_lua(script_file_path_and_name));

        constexpr const char* global_register_func_name = "Register";
        constexpr const char* global_on_match_found_func_name = "OnMatchFound";

        if (!lua.is_global_function(global_register_func_name) || !lua.is_global_function(global_on_match_found_func_name))
        {
            Output::send(SYSSTR("Lua functions 'Register' and 'OnMatchFound' must be present in {}\n"), script_file_path_and_name);
            throw std::runtime_error{"See error message above"};
        }

        lua.call_function(global_register_func_name, 0, 1);

        if (!lua.is_string())
        {
            throw std::runtime_error{"Lua function 'Register' must return a string that contains the signature to scan for"};
        }

        signature_containers.emplace_back(SignatureContainer{{
                                                                     {lua.get_string().data()},
                                                             },
                                                             // On Match Found
                                                             [&lua, match_found_func](SignatureContainer& self) -> bool {
                                                                 lua.prepare_function_call(global_on_match_found_func_name);
                                                                 lua.set_integer(reinterpret_cast<uintptr_t>(static_cast<void*>(self.get_match_address())));
                                                                 lua.call_function(1, 1);

                                                                 if (!lua.is_integer())
                                                                 {
                                                                     return false;
                                                                 }

                                                                 void* found_address = reinterpret_cast<void*>(lua.get_integer());
                                                                 if (!found_address)
                                                                 {
                                                                     return false;
                                                                 }

                                                                 DidLuaScanSucceed did_lua_scan_succeed = match_found_func(found_address);

                                                                 if (did_lua_scan_succeed == DidLuaScanSucceed::Yes)
                                                                 {
                                                                     self.get_did_succeed() = true;
                                                                     return true;
                                                                 }
                                                                 else
                                                                 {
                                                                     return false;
                                                                 }
                                                             },
                                                             // On Scan Completed
                                                             [scan_complete_func]([[maybe_unused]] const SignatureContainer& self) {
                                                                 scan_complete_func(self.get_did_succeed() ? DidLuaScanSucceed::Yes : DidLuaScanSucceed::No);
                                                             }});

        // lua_close(lua.get_lua_state());
    }

    auto setup_lua_scan_overrides(std::filesystem::path& working_directory, Unreal::UnrealInitializer::Config& config) -> void
    {
        auto lua_guobjectarray_scan_script = to_system(working_directory / "UE4SS_Signatures/GUObjectArray.lua");
        if (std::filesystem::exists(lua_guobjectarray_scan_script))
        {
            config.ScanOverrides.guobjectarray = [lua_guobjectarray_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                                 Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_guobjectarray_scan_script,
                        signature_containers,
                        [](void* address) {
                            Output::send(SYSSTR("GUObjectArray address: {} <- Lua Script\n"), address);
                            Unreal::UObjectArray::SetupGUObjectArrayAddress(address);
                            return DidLuaScanSucceed::Yes;
                        },
                        [&](DidLuaScanSucceed did_lua_scan_succeed) {
                            if (did_lua_scan_succeed == DidLuaScanSucceed::No)
                            {
                                scan_result.Errors.emplace_back("Was unable to find AOB for 'GUObjectArray' via Lua script");
                            }
                        });
            };
        }

        auto lua_fts_scan_script = to_system(working_directory / "UE4SS_Signatures/FName_ToString.lua");
        if (std::filesystem::exists(lua_fts_scan_script))
        {
            config.ScanOverrides.fname_to_string = [lua_fts_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                         Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_fts_scan_script,
                        signature_containers,
                        [](void* address) {
                            Output::send(SYSSTR("FName::ToString address: {} <- Lua Script\n"), address);
                            Unreal::FName::ToStringInternal.assign_address(address);
                            return DidLuaScanSucceed::Yes;
                        },
                        [&](DidLuaScanSucceed did_lua_scan_succeed) {
                            if (did_lua_scan_succeed == DidLuaScanSucceed::No)
                            {
                                scan_result.Errors.emplace_back("Was unable to find AOB for 'FName::ToString' via Lua script");
                            }
                        });
            };
        }

        auto lua_fnc_scan_script = to_system(working_directory / "UE4SS_Signatures/FName_Constructor.lua");
        if (std::filesystem::exists(lua_fnc_scan_script))
        {
            config.ScanOverrides.fname_constructor = [lua_fnc_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                           Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_fnc_scan_script,
                        signature_containers,
                        [&scan_result](void* address) {
                            Unreal::FName name = Unreal::FName(STR("bCanBeDamaged"), Unreal::FNAME_Find, address);

                            if (name == STR("bCanBeDamaged"))
                            {
                                Output::send(SYSSTR("FName::FName address: {} <- Lua Script\n"), address);
                                Unreal::FName::ConstructorInternal.assign_address(address);
                                return DidLuaScanSucceed::Yes;
                            }
                            else
                            {
                                scan_result.Errors.emplace_back("Lua script 'FName_Constructor.lua' did not return a valid address for FName::FName.");
                                return DidLuaScanSucceed::No;
                            }
                        },
                        [&scan_result]([[maybe_unused]] DidLuaScanSucceed did_lua_scan_succeed) {
                            if (!Unreal::FName::ConstructorInternal.get_function_address())
                            {
                                scan_result.Errors.emplace_back("Lua script 'FName_Constructor.lua' did not return a valid address for FName::FName.");
                            }
                        });
            };
        }

        // For compatibility, we look for 'FMemory_Free.lua' if 'GMalloc.lua' doesn't exist.
        auto lua_ffree_scan_script_new = to_system(working_directory / "UE4SS_Signatures/GMalloc.lua");
        auto lua_ffree_scan_script_compat = to_system(working_directory / "UE4SS_Signatures/FMemory_Free.lua");
        auto lua_ffree_scan_script = std::filesystem::exists(lua_ffree_scan_script_new) ? lua_ffree_scan_script_new : lua_ffree_scan_script_compat;
        if (std::filesystem::exists(lua_ffree_scan_script))
        {
            config.ScanOverrides.fmemory_free = [lua_ffree_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                        Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_ffree_scan_script,
                        signature_containers,
                        [](void* address) {
                            Output::send(SYSSTR("GMalloc address: {} <- Lua Script\n"), address);
                            Unreal::FMalloc::UnrealStaticGMalloc = static_cast<Unreal::FMalloc**>(address);
                            Unreal::GMalloc = *Unreal::FMalloc::UnrealStaticGMalloc;
                            return DidLuaScanSucceed::Yes;
                        },
                        [&](DidLuaScanSucceed did_lua_scan_succeed) {
                            if (did_lua_scan_succeed == DidLuaScanSucceed::No)
                            {
                                scan_result.Errors.emplace_back("Was unable to find AOB for 'GMalloc' via Lua script");
                            }
                        });
            };
        }

        auto lua_sco_scan_script = to_system(working_directory / "UE4SS_Signatures/StaticConstructObject.lua");
        if (std::filesystem::exists(lua_sco_scan_script))
        {
            config.ScanOverrides.static_construct_object = [lua_sco_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                                 Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_sco_scan_script,
                        signature_containers,
                        [](void* address) {
                            Output::send(SYSSTR("StaticConstructObject_Internal address: {} <- Lua Script\n"), address);
                            Unreal::UObjectGlobals::SetupStaticConstructObjectInternalAddress(address);
                            return DidLuaScanSucceed::Yes;
                        },
                        [&](DidLuaScanSucceed did_lua_scan_succeed) {
                            if (did_lua_scan_succeed == DidLuaScanSucceed::No)
                            {
                                scan_result.Errors.emplace_back("Was unable to find AOB for 'StaticConstructObject' via Lua script");
                            }
                        });
            };
        }

        auto lua_ftc_scan_script = to_system(working_directory / "UE4SS_Signatures/FText_Constructor.lua");
        if (std::filesystem::exists(lua_ftc_scan_script))
        {
            config.ScanOverrides.ftext_constructor = [lua_ftc_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                           Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_ftc_scan_script,
                        signature_containers,
                        [&scan_result](void* address) {
                            Unreal::FText text = Unreal::FText(STR("bCanBeDamaged"), address);

                            if (text == STR("bCanBeDamaged"))
                            {
                                Output::send(SYSSTR("FText::FText address: {} <- Lua Script\n"), address);
                                Unreal::FText::ConstructorInternal.assign_address(address);
                                return DidLuaScanSucceed::Yes;
                            }
                            else
                            {
                                scan_result.Errors.emplace_back("Lua script 'FText_Constructor.lua' did not return a valid address for FText::FText.");
                                return DidLuaScanSucceed::No;
                            }
                        },
                        [&scan_result]([[maybe_unused]] DidLuaScanSucceed did_lua_scan_succeed) {
                            if (!Unreal::FText::ConstructorInternal.get_function_address())
                            {
                                scan_result.Errors.emplace_back("Lua script 'FText_Constructor.lua' did not return a valid address for FText::FText.");
                            }
                        });
            };
        }
    }
} // namespace RC

#include <filesystem>

#include <DynamicOutput/DynamicOutput.hpp>
#include <LuaLibrary.hpp>
#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include <SigScanner/SinglePassSigScanner.hpp>
#include <Signatures.hpp>
#include <ExceptionHandling.hpp>
#include <Unreal/FMemory.hpp>
#include <Unreal/FString.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/Script.hpp>
#include <Unreal/Signatures.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UnrealInitializer.hpp>

#include <Helpers/String.hpp>

// Required for SEH_TRY and SEH_EXCEPT.
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace RC
{
    auto scan_complete_default_func(DidLuaScanSucceed) -> void
    {
    }

    auto scan_from_lua_script(std::filesystem::path& script_file_path_and_name,
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
        lua.register_function("LoadExport", LuaLibrary::load_export);
        lua.register_function("loadexport", LuaLibrary::load_export);

        lua.execute_file(script_file_path_and_name.string());

        if (lua.get_stack_size() > 0)
        {
            if (lua.is_integer())
            {
                auto found_address = reinterpret_cast<void*>(lua.get_integer());
                match_found_func(found_address);
                return;
            }
            else if (lua.is_nil())
            {
                throw std::runtime_error{fmt::format("Lua file returned nil (symbol not found): {}", to_string(script_file_path_and_name))};
            }
        }

        constexpr const char* global_register_func_name = "Register";
        constexpr const char* global_on_match_found_func_name = "OnMatchFound";

        if (!lua.is_global_function(global_register_func_name) || !lua.is_global_function(global_on_match_found_func_name))
        {
            Output::send(STR("Lua functions 'Register' and 'OnMatchFound' must be "
                             "present in {}\n"),
                         ensure_str(script_file_path_and_name));
            throw std::runtime_error{"See error message above"};
        }

        lua.call_function(global_register_func_name, 0, 1);

        if (!lua.is_string())
        {
            throw std::runtime_error{"Lua function 'Register' must return a string "
                                     "that contains the signature to scan for"};
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
        auto lua_guobjectarray_scan_script = working_directory / "UE4SS_Signatures/GUObjectArray.lua";
        if (std::filesystem::exists(lua_guobjectarray_scan_script))
        {
            config.ScanOverrides.guobjectarray = [lua_guobjectarray_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                                 Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_guobjectarray_scan_script,
                        signature_containers,
                        [](void* address) {
                            Output::send(STR("GUObjectArray address: {} <- Lua Script\n"), address);
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

        auto lua_fts_scan_script = working_directory / "UE4SS_Signatures/FName_ToString.lua";
        if (std::filesystem::exists(lua_fts_scan_script))
        {
            config.ScanOverrides.fname_to_string = [lua_fts_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                         Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_fts_scan_script,
                        signature_containers,
                        [](void* address) {
                            Output::send(STR("FName::ToString address: {} <- Lua Script\n"), address);
                            Unreal::FName::ToStringInternal.assign_address(address);
                            return DidLuaScanSucceed::Yes;
                        },
                        [&](DidLuaScanSucceed did_lua_scan_succeed) {
                            if (did_lua_scan_succeed == DidLuaScanSucceed::No)
                            {
                                scan_result.Errors.emplace_back("Was unable to find AOB for 'FName::ToString' via Lua "
                                                                "script");
                            }
                        });
            };
        }

        auto lua_fnc_scan_script = working_directory / "UE4SS_Signatures/FName_Constructor.lua";
        if (std::filesystem::exists(lua_fnc_scan_script))
        {
            config.ScanOverrides.fname_constructor = [lua_fnc_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                           Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_fnc_scan_script,
                        signature_containers,
                        [&scan_result](void* address) {
                            Unreal::FName name{};
                            SEH_TRY({ name = Unreal::FName(STR("bCanBeDamaged"), Unreal::FNAME_Find, address); })
                            SEH_EXCEPT({ Output::send<LogLevel::Error>(STR("Error: Crashed calling FName constructor.\n")); });

                            DidLuaScanSucceed did_succeed{};
                            SEH_TRY({
                                if (name == STR("bCanBeDamaged"))
                                {
                                    Output::send(STR("FName::FName address: {} <- Lua Script\n"), address);
                                    Unreal::FName::ConstructorInternal.assign_address(address);
                                    did_succeed = DidLuaScanSucceed::Yes;
                                }
                                else
                                {
                                    scan_result.Errors.emplace_back("Lua script 'FName_Constructor.lua' did not return a "
                                                                    "valid address for FName::FName.");
                                    did_succeed = DidLuaScanSucceed::No;
                                }
                            })
                            SEH_EXCEPT({ Output::send<LogLevel::Error>(STR("Error: Crashed calling FName::ToString.\n")); })
                            return did_succeed;
                        },
                        [&scan_result]([[maybe_unused]] DidLuaScanSucceed did_lua_scan_succeed) {
                            if (!Unreal::FName::ConstructorInternal.get_function_address())
                            {
                                scan_result.Errors.emplace_back("Lua script 'FName_Constructor.lua' did not return a "
                                                                "valid address for FName::FName.");
                            }
                        });
            };
        }

        // For compatibility, we look for 'FMemory_Free.lua' if 'GMalloc.lua' doesn't
        // exist.
        auto lua_ffree_scan_script_new = working_directory / "UE4SS_Signatures/GMalloc.lua";
        auto lua_ffree_scan_script_compat = working_directory / "UE4SS_Signatures/FMemory_Free.lua";
        auto lua_ffree_scan_script = std::filesystem::exists(lua_ffree_scan_script_new) ? lua_ffree_scan_script_new : lua_ffree_scan_script_compat;
        if (std::filesystem::exists(lua_ffree_scan_script))
        {
            config.ScanOverrides.fmemory_free = [lua_ffree_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                        Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_ffree_scan_script,
                        signature_containers,
                        [](void* address) {
                            Output::send(STR("GMalloc address: {} <- Lua Script\n"), address);
                            Unreal::GMalloc = static_cast<Unreal::FMalloc**>(address);
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

        auto lua_sco_scan_script = working_directory / "UE4SS_Signatures/StaticConstructObject.lua";
        if (std::filesystem::exists(lua_sco_scan_script))
        {
            config.ScanOverrides.static_construct_object = [lua_sco_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                                 Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_sco_scan_script,
                        signature_containers,
                        [](void* address) {
                            Output::send(STR("StaticConstructObject_Internal address: {} "
                                             "<- Lua Script\n"),
                                         address);
                            Unreal::UObjectGlobals::SetupStaticConstructObjectInternalAddress(address);
                            return DidLuaScanSucceed::Yes;
                        },
                        [&](DidLuaScanSucceed did_lua_scan_succeed) {
                            if (did_lua_scan_succeed == DidLuaScanSucceed::No)
                            {
                                scan_result.Errors.emplace_back("Was unable to find AOB for 'StaticConstructObject' via "
                                                                "Lua script");
                            }
                        });
            };
        }

        auto lua_ftc_scan_script = working_directory / "UE4SS_Signatures/FText_Constructor.lua";
        if (std::filesystem::exists(lua_ftc_scan_script))
        {
            config.ScanOverrides.ftext_constructor = [lua_ftc_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                           Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_ftc_scan_script,
                        signature_containers,
                        [&scan_result](void* address) {
                            if (!Unreal::GMalloc || !*Unreal::GMalloc)
                            {
                                    scan_result.Errors.emplace_back("Lua script 'FText_Constructor.lua' cannot be tried; GMalloc nullptr.");
                                    return DidLuaScanSucceed::No;
                            }
                            Unreal::FText text{};
                            SEH_TRY({ text = Unreal::FText(STR("bCanBeDamaged"), address); })
                            SEH_EXCEPT({ Output::send<LogLevel::Error>(STR("Error: Crashed calling FText constructor.\n")); });

                            DidLuaScanSucceed did_succeed{};
                            SEH_TRY({
                                if (text == STR("bCanBeDamaged"))
                                {
                                    Output::send(STR("FText::FText address: {} <- Lua Script\n"), address);
                                    Unreal::FText::ConstructorInternal.assign_address(address);
                                    did_succeed = DidLuaScanSucceed::Yes;
                                }
                                else
                                {
                                    scan_result.Errors.emplace_back("Lua script 'FText_Constructor.lua' did not return a "
                                                                    "valid address for FText::FText.");
                                    did_succeed = DidLuaScanSucceed::No;
                                }
                            })
                            SEH_EXCEPT({ Output::send<LogLevel::Error>(STR("Error: Crashed calling FText::ToString.\n")); })
                            return did_succeed;
                        },
                        [&scan_result]([[maybe_unused]] DidLuaScanSucceed did_lua_scan_succeed) {
                            if (!Unreal::FText::ConstructorInternal.get_function_address())
                            {
                                scan_result.Errors.emplace_back("Lua script 'FText_Constructor.lua' did not return a "
                                                                "valid address for FText::FText.");
                            }
                        });
            };
        }

        auto lua_guhashtables_scan_script = working_directory / "UE4SS_Signatures/GUObjectHashTables.lua";
        if (std::filesystem::exists(lua_guhashtables_scan_script))
        {
            config.ScanOverrides.fuobject_hash_tables_get = [lua_guhashtables_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                                           Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_guhashtables_scan_script,
                        signature_containers,
                        [](void* address) {
                            Output::send(STR("GUObjectHashTables_Get address: {} <- Lua Script\n"), address);

                            return DidLuaScanSucceed::Yes;
                        },
                        [&](DidLuaScanSucceed did_lua_scan_succeed) {
                            if (did_lua_scan_succeed == DidLuaScanSucceed::No)
                            {
                                scan_result.Errors.emplace_back("Was unable to find AOB for 'GUObjectHashTables' via Lua "
                                                                "script");
                            }
                        });
            };
        }

        auto lua_gnatives_scan_script = working_directory / "UE4SS_Signatures/GNatives.lua";
        if (std::filesystem::exists(lua_gnatives_scan_script))
        {
            config.ScanOverrides.gnatives = [lua_gnatives_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                       Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_gnatives_scan_script,
                        signature_containers,
                        [](void* address) {
                            Output::send(STR("GNatives address: {} <- Lua Script\n"), address);
                            Unreal::GNatives_Internal = reinterpret_cast<Unreal::FNativeFuncPtr*>(address);
                            return DidLuaScanSucceed::Yes;
                        },
                        [&](DidLuaScanSucceed did_lua_scan_succeed) {
                            if (did_lua_scan_succeed == DidLuaScanSucceed::No)
                            {
                                scan_result.Errors.emplace_back("Was unable to find AOB for 'GNatives' via Lua script");
                            }
                        });
            };
        }

        auto lua_consolemanager_scan_script = working_directory / "UE4SS_Signatures/ConsoleManager.lua";
        if (std::filesystem::exists(lua_consolemanager_scan_script))
        {
            config.ScanOverrides.console_manager_singleton = [lua_consolemanager_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                                              Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_consolemanager_scan_script,
                        signature_containers,
                        [](void* address) {
                            Output::send(STR("ConsoleManagerSingleton address: {} <- Lua Script\n"), address);

                            return DidLuaScanSucceed::Yes;
                        },
                        [&](DidLuaScanSucceed did_lua_scan_succeed) {
                            if (did_lua_scan_succeed == DidLuaScanSucceed::No)
                            {
                                scan_result.Errors.emplace_back("Was unable to find AOB for 'ConsoleManagerSingleton' "
                                                                "via Lua script");
                            }
                        });
            };
        }

        auto lua_process_local_script_function_scan_script = working_directory / "UE4SS_Signatures/ProcessLocalScriptFunction.lua";
        if (std::filesystem::exists(lua_process_local_script_function_scan_script))
        {
            config.ScanOverrides.process_local_script_function =
                    [lua_process_local_script_function_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                    Unreal::Signatures::ScanResult& scan_result) mutable {
                        scan_from_lua_script(
                                lua_process_local_script_function_scan_script,
                                signature_containers,
                                [](void* address) {
                                    Output::send(STR("ProcessLocalScriptFunction address: {} <- "
                                                     "Lua Script\n"),
                                                 address);
                                    Unreal::UObject::ProcessLocalScriptFunctionInternal.assign_address(address);
                                    return DidLuaScanSucceed::Yes;
                                },
                                [&](DidLuaScanSucceed did_lua_scan_succeed) {
                                    if (did_lua_scan_succeed == DidLuaScanSucceed::No)
                                    {
                                        scan_result.Errors.emplace_back("Was unable to find AOB for 'ProcessLocalScriptFunction' "
                                                                        "via Lua script");
                                    }
                                });
                    };
        }

        auto lua_process_internal_scan_script = working_directory / "UE4SS_Signatures/ProcessInternal.lua";
        if (std::filesystem::exists(lua_process_internal_scan_script))
        {
            config.ScanOverrides.process_internal = [lua_process_internal_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                                       Unreal::Signatures::ScanResult& scan_result) mutable {
                scan_from_lua_script(
                        lua_process_internal_scan_script,
                        signature_containers,
                        [](void* address) {
                            Output::send(STR("ProcessInternal address: {} <- Lua Script\n"), address);
                            Unreal::UObject::ProcessInternalInternal.assign_address(address);
                            return DidLuaScanSucceed::Yes;
                        },
                        [&](DidLuaScanSucceed did_lua_scan_succeed) {
                            if (did_lua_scan_succeed == DidLuaScanSucceed::No)
                            {
                                scan_result.Errors.emplace_back("Was unable to find AOB for 'ProcessInternal' via Lua "
                                                                "script");
                            }
                        });
            };
        }

        auto lua_call_function_by_name_with_arguments_scan_script = working_directory / "UE4SS_Signatures/CallFunctionByNameWithArguments.lua";
        if (std::filesystem::exists(lua_call_function_by_name_with_arguments_scan_script))
        {
            config.ScanOverrides.call_function_by_name_with_arguments =
                    [lua_call_function_by_name_with_arguments_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                           Unreal::Signatures::ScanResult& scan_result) mutable {
                        scan_from_lua_script(
                                lua_call_function_by_name_with_arguments_scan_script,
                                signature_containers,
                                [](void* address) {
                                    Output::send(STR("CallFunctionByNameWithArguments address: {} "
                                                     "<- Lua Script\n"),
                                                 address);
                                    Unreal::UObject::CallFunctionByNameWithArgumentsInternal.assign_address(address);
                                    return DidLuaScanSucceed::Yes;
                                },
                                [&](DidLuaScanSucceed did_lua_scan_succeed) {
                                    if (did_lua_scan_succeed == DidLuaScanSucceed::No)
                                    {
                                        scan_result.Errors.emplace_back("Was unable to find AOB for "
                                                                        "'CallFunctionByNameWithArguments' via Lua script");
                                    }
                                });
                    };
        }

        auto lua_gameengine_tick_scan_script = working_directory / "UE4SS_Signatures/GameEngineTick.lua";
        if (std::filesystem::exists(lua_gameengine_tick_scan_script))
        {
            config.ScanOverrides.gameengine_tick =
                    [lua_gameengine_tick_scan_script](std::vector<SignatureContainer>& signature_containers,
                                                                           Unreal::Signatures::ScanResult& scan_result) mutable {
                        scan_from_lua_script(
                                lua_gameengine_tick_scan_script,
                                signature_containers,
                                [](void* address) {
                                    Output::send(STR("GameEngine::Tick address: {} "
                                                     "<- Lua Script\n"),
                                                 address);
                                    Unreal::UEngine::TickInternal.assign_address(address);
                                    return DidLuaScanSucceed::Yes;
                                },
                                [&](DidLuaScanSucceed did_lua_scan_succeed) {
                                    if (did_lua_scan_succeed == DidLuaScanSucceed::No)
                                    {
                                        scan_result.Errors.emplace_back("Was unable to find AOB for "
                                                                        "'GameEngine::Tick' via Lua script");
                                    }
                                });
                    };
        }
    }
} // namespace RC

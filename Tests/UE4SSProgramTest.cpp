#define NOMINMAX

#include <limits>
#include <type_traits>
#include "UE4SSProgramTest.hpp"
#include <Unreal/TypeChecker.hpp>
#include <UnrealDef.hpp>
#include <Timer/ScopedTimer.hpp>
#include <Timer/FunctionTimer.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Unreal/Hooks.hpp>

#define assert_test(expression, notification) \
if (!(expression)) { throw std::runtime_error{fmt("Test failed -> %S",  notification)}; }

namespace RC
{
    enum class TestableEnumClass
    {
        ClassValueOne,
        ClassValueTwo,
        ClassValueThree,
    };

    class ACharacter : public Unreal::UObject
    {
        ACharacter() = default;

    public:
        struct CanJumpInternal_Params {};
        auto CanJumpInternal() -> bool
        {
            CanJumpInternal_Params params{};
            UFunction* function = UObjectGlobals::static_find_object<UFunction*>(nullptr, nullptr, L"/Script/Engine.Character:CanJumpInternal");

            process_event(function, &params);
        }
    };

    class UCharacterMovementComponent : public Unreal::UObject
    {
        UCharacterMovementComponent() = default;

    public:
        struct SetMovementMode_Params
        {
            uint8_t NewMovementMode;
            uint8_t NewCustomMode;
        };

        auto SetMovementMode(uint8_t NewMovementMode, uint8_t NewCustomMode) -> void
        {
            SetMovementMode_Params params{NewMovementMode, NewCustomMode};
            UFunction* function = UObjectGlobals::static_find_object<UFunction*>(nullptr, nullptr, L"/Script/Engine.CharacterMovementComponent:SetMovementMode");

            process_event(function, &params);
        }
    };

    UE4SSProgramTest::UE4SSProgramTest(const std::wstring& moduleFilePath, std::initializer_list<BinaryOptions> options) : UE4SSProgram(moduleFilePath, options)
    {
        // Don't execute this test program if we already have errors
        if (auto e = get_error_object(); e->has_error()) { return; }

        try
        {
            m_input_handler.register_keydown_event(Input::Key::K, [&]() {
                Output::send(STR("Executing function call tests...\n"));
                execute_function_call_tests();
                Output::send(STR("Function call tests executed\n"));
            });

            m_input_handler.register_keydown_event(Input::Key::HOME, [&]() {
                Output::send(STR("VK_HOME down event\n"));
            });

            m_input_handler.register_keydown_event(Input::Key::J, [&]() {
                Output::send(STR("J was hit\n"));
            });

            m_input_handler.register_keydown_event(Input::Key::J, {Input::ModifierKey::ALT}, [&]() {
                Output::send(STR("ALT + J was hit\n"));
            });

            // Input handler, multi-key tests -> START
            m_input_handler.register_keydown_event(Input::Key::P, [](){
                Output::send(STR("P was hit\n"));
            });

            m_input_handler.register_keydown_event(Input::Key::P, [](){
                Output::send(STR("P was hit / event two\n"));
            });

            m_input_handler.register_keydown_event(Input::Key::P, {Input::ModifierKey::ALT}, [](){
                Output::send(STR("ALT + P was hit\n"));
            });

            m_input_handler.register_keydown_event(Input::Key::P, {Input::ModifierKey::CONTROL}, [](){
                Output::send(STR("Dumping all timed functions\n"));
                FunctionTimerCollection::dump();
            });

            // This one triggers both the "ALT + P" & "CTRL + ALT + P" keydown events
            // All the other events seem fine, so for now we'll just avoid using more than one modifier key
            m_input_handler.register_keydown_event(Input::Key::P, {Input::ModifierKey::CONTROL, Input::ModifierKey::ALT}, [](){
                Output::send(STR("CTRL + ALT + P was hit\n"));
            });

            Output::send(STR("Executing generic tests...\n"));
            execute_tests();
            Output::send(STR("Generic tests executed\n"));

            // Program is now fully setup
            // Start event loop
            m_event_loop = std::jthread{&UE4SSProgramTest::update, this};

            // Wait for thread
            // There's a loop inside the thread that only exits when you hit the 'End' key on the keyboard
            // As long as you don't do that the thread will stay open and accept further inputs
            m_event_loop.join();
        }
        catch (std::runtime_error& e)
        {
            // Returns to main from here which checks, displays & handles whether to close the program or not
            // If has_error() returns false that means that set_error was not called
            // In that case we need to copy the exception message to the error buffer before we return to main
            if (!m_error_object->has_error())
            {
                copy_error_into_message(e.what());

            }
            return;
        }
    }

    auto UE4SSProgramTest::execute_tests() -> void
    {
        TIME_FUNCTION()
        // For testing only!!!

        Output::send(STR("##### CORE OBJECTS DUMP -> START #####\n\n"));
        Unreal::TypeChecker::dump_all_stored_object_types(
                [](uint32_t index, uint32_t number) {
                    FName name{index, number};
                    Output::send(STR("Name: {:#x} | Number: {:#x} | ToString(): {}\n"), index, number, name.to_string());
                },
                [](const std::wstring& obj_name, void* obj_ptr) {
                    Output::send(STR("ptr: {} | obj_name: {}\n"), obj_ptr, obj_name);
                }
        );
        Output::send(STR("\n##### CORE OBJECTS DUMP -> END #####\n\n"));

        auto dump_property_flags = [](Unreal::EPropertyFlags flags) {
            std::wstring all_flags{};

            // (get_property_flags() & flags_to_check) != 0 || flags_to_check == CPF_AllFlags;

            if ((flags & Unreal::EPropertyFlags::CPF_None) != 0) { all_flags.append(STR("CPF_None | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_Edit) != 0) { all_flags.append(STR("CPF_Edit | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_ConstParm) != 0) { all_flags.append(STR("CPF_ConstParm | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_BlueprintVisible) != 0) { all_flags.append(STR("CPF_BlueprintVisible | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_ExportObject) != 0) { all_flags.append(STR("CPF_ExportObject | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_BlueprintReadOnly) != 0) { all_flags.append(STR("CPF_BlueprintReadOnly | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_Net) != 0) { all_flags.append(STR("CPF_Net | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_EditFixedSize) != 0) { all_flags.append(STR("CPF_EditFixedSize | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_Parm) != 0) { all_flags.append(STR("CPF_Parm | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_OutParm) != 0) { all_flags.append(STR("CPF_OutParm | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_ZeroConstructor) != 0) { all_flags.append(STR("CPF_ZeroConstructor | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_ReturnParm) != 0) { all_flags.append(STR("CPF_ReturnParm | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_DisableEditOnTemplate) != 0) { all_flags.append(STR("CPF_DisableEditOnTemplate | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_Transient) != 0) { all_flags.append(STR("CPF_Transient | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_Config) != 0) { all_flags.append(STR("CPF_Config | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_DisableEditOnInstance) != 0) { all_flags.append(STR("CPF_DisableEditOnInstance | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_EditConst) != 0) { all_flags.append(STR("CPF_EditConst | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_GlobalConfig) != 0) { all_flags.append(STR("CPF_GlobalConfig | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_InstancedReference) != 0) { all_flags.append(STR("CPF_InstancedReference | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_DuplicateTransient) != 0) { all_flags.append(STR("CPF_DuplicateTransient | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_SubobjectReference) != 0) { all_flags.append(STR("CPF_SubobjectReference | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_SaveGame) != 0) { all_flags.append(STR("CPF_SaveGame | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_NoClear) != 0) { all_flags.append(STR("CPF_NoClear | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_ReferenceParm) != 0) { all_flags.append(STR("CPF_ReferenceParm | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_BlueprintAssignable) != 0) { all_flags.append(STR("CPF_BlueprintAssignable | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_Deprecated) != 0) { all_flags.append(STR("CPF_Deprecated | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_IsPlainOldData) != 0) { all_flags.append(STR("CPF_IsPlainOldData | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_RepSkip) != 0) { all_flags.append(STR("CPF_RepSkip | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_RepNotify) != 0) { all_flags.append(STR("CPF_RepNotify | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_Interp) != 0) { all_flags.append(STR("CPF_Interp | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_NonTransactional) != 0) { all_flags.append(STR("CPF_NonTransactional | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_EditorOnly) != 0) { all_flags.append(STR("CPF_EditorOnly | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_NoDestructor) != 0) { all_flags.append(STR("CPF_NoDestructor | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_AutoWeak) != 0) { all_flags.append(STR("CPF_AutoWeak | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_ContainsInstancedReference) != 0) { all_flags.append(STR("CPF_ContainsInstancedReference | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_AssetRegistrySearchable) != 0) { all_flags.append(STR("CPF_AssetRegistrySearchable | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_SimpleDisplay) != 0) { all_flags.append(STR("CPF_SimpleDisplay | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_AdvancedDisplay) != 0) { all_flags.append(STR("CPF_AdvancedDisplay | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_Protected) != 0) { all_flags.append(STR("CPF_Protected | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_BlueprintCallable) != 0) { all_flags.append(STR("CPF_BlueprintCallable | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_BlueprintAuthorityOnly) != 0) { all_flags.append(STR("CPF_BlueprintAuthorityOnly | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_TextExportTransient) != 0) { all_flags.append(STR("CPF_TextExportTransient | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_NonPIEDuplicateTransient) != 0) { all_flags.append(STR("CPF_NonPIEDuplicateTransient | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_ExposeOnSpawn) != 0) { all_flags.append(STR("CPF_ExposeOnSpawn | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_PersistentInstance) != 0) { all_flags.append(STR("CPF_PersistentInstance | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_UObjectWrapper) != 0) { all_flags.append(STR("CPF_UObjectWrapper | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_HasGetValueTypeHash) != 0) { all_flags.append(STR("CPF_HasGetValueTypeHash | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_NativeAccessSpecifierPublic) != 0) { all_flags.append(STR("CPF_NativeAccessSpecifierPublic | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_NativeAccessSpecifierProtected) != 0) { all_flags.append(STR("CPF_NativeAccessSpecifierProtected | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_NativeAccessSpecifierPrivate) != 0) { all_flags.append(STR("CPF_NativeAccessSpecifierPrivate | ")); }
            if ((flags & Unreal::EPropertyFlags::CPF_SkipSerialization) != 0) { all_flags.append(STR("CPF_SkipSerialization | ")); }

            Output::send(STR("property_flags: {}\n"), all_flags);
        };

        Unreal::EPropertyFlags prop_flags_one{0x0010001040000205};
        Unreal::EPropertyFlags prop_flags_two{0x0018001040000005};
        Unreal::EPropertyFlags prop_flags_three{0x0018001040000205};
        Unreal::EPropertyFlags prop_flags_four{0x0018001040000205};
        dump_property_flags(prop_flags_one);
        dump_property_flags(prop_flags_two);
        dump_property_flags(prop_flags_three);
        dump_property_flags(prop_flags_four);

        auto dump_flags = [](EObjectFlags flags) {
            std::wstring all_flags{};

            if (flags & EObjectFlags::RF_NoFlags) { all_flags.append(STR("NoFlags | ")); }
            if (flags & EObjectFlags::RF_Public) { all_flags.append(STR("Public | ")); }
            if (flags & EObjectFlags::RF_Standalone) { all_flags.append(STR("Standalone | ")); }
            if (flags & EObjectFlags::RF_MarkAsNative) { all_flags.append(STR("MarkAsNative | ")); }
            if (flags & EObjectFlags::RF_Transactional) { all_flags.append(STR("Transactional | ")); }
            if (flags & EObjectFlags::RF_ClassDefaultObject) { all_flags.append(STR("ClassDefaultObject | ")); }
            if (flags & EObjectFlags::RF_ArchetypeObject) { all_flags.append(STR("ArchetypeObject | ")); }
            if (flags & EObjectFlags::RF_Transient) { all_flags.append(STR("Transient | ")); }
            if (flags & EObjectFlags::RF_MarkAsRootSet) { all_flags.append(STR("MarkAsRootSet | ")); }
            if (flags & EObjectFlags::RF_TagGarbageTemp) { all_flags.append(STR("TagGarbageTemp | ")); }
            if (flags & EObjectFlags::RF_NeedInitialization) { all_flags.append(STR("NeedInitialization | ")); }
            if (flags & EObjectFlags::RF_NeedLoad) { all_flags.append(STR("NeedLoad | ")); }
            if (flags & EObjectFlags::RF_KeepForCooker) { all_flags.append(STR("KeepForCooker | ")); }
            if (flags & EObjectFlags::RF_NeedPostLoad) { all_flags.append(STR("NeedPostLoad | ")); }
            if (flags & EObjectFlags::RF_NeedPostLoadSubobjects) { all_flags.append(STR("NeedPostLoadSubobjects | ")); }
            if (flags & EObjectFlags::RF_NewerVersionExists) { all_flags.append(STR("NewerVersionExists | ")); }
            if (flags & EObjectFlags::RF_BeginDestroyed) { all_flags.append(STR("BeginDestroyed | ")); }
            if (flags & EObjectFlags::RF_FinishDestroyed) { all_flags.append(STR("FinishDestroyed | ")); }
            if (flags & EObjectFlags::RF_BeingRegenerated) { all_flags.append(STR("BeingRegenerated | ")); }
            if (flags & EObjectFlags::RF_DefaultSubObject) { all_flags.append(STR("DefaultSubObject | ")); }
            if (flags & EObjectFlags::RF_WasLoaded) { all_flags.append(STR("WasLoaded | ")); }
            if (flags & EObjectFlags::RF_TextExportTransient) { all_flags.append(STR("TextExportTransient | ")); }
            if (flags & EObjectFlags::RF_LoadCompleted) { all_flags.append(STR("LoadCompleted | ")); }
            if (flags & EObjectFlags::RF_InheritableComponentTemplate) { all_flags.append(STR("InheritableComponentTemplate | ")); }
            if (flags & EObjectFlags::RF_DuplicateTransient) { all_flags.append(STR("DuplicateTransient | ")); }
            if (flags & EObjectFlags::RF_StrongRefOnFrame) { all_flags.append(STR("StrongRefOnFrame | ")); }
            if (flags & EObjectFlags::RF_NonPIEDuplicateTransient) { all_flags.append(STR("NonPIEDuplicateTransient | ")); }
            if (flags & EObjectFlags::RF_Dynamic) { all_flags.append(STR("Dynamic | ")); }
            if (flags & EObjectFlags::RF_WillBeLoaded) { all_flags.append(STR("WillBeLoaded | ")); }

            Output::send(STR("\tget_flags(): {:#X} | {}\n"), static_cast<uint32_t>(flags), all_flags);
        };

        auto dump_obj_flags = [&]([[maybe_unused]]UObject* obj) {
            //std::string all_flags{};

            //auto flags = obj->get_flags();
            //dump_flags(flags);
        };

        auto dump_properties = [](UObject* obj) {
            constexpr uint32_t max_props_to_dump{4};
            uint32_t num_props_dumped{};


            UClass* obj_as_class = reinterpret_cast<UClass*>(obj);
            for (XProperty* children = obj_as_class->get_childproperties<XProperty*>(); children && num_props_dumped < max_props_to_dump; children = children->get_next<XProperty*>())
            {
                XProperty* type_ptr = std::bit_cast<XProperty*>(children->static_class().to_field());

                // Can't use GetFullName until the FField version has been implemented
                Output::send(STR("\t\t{} {{\n"), children->get_fname().to_string().c_str());
                Output::send(STR("\t\t\ttype: {} ({})\n"), (void*)children->get_ffieldclass(), children->get_type_fname().to_string());
                // Yes this is a very bizarre place to put '::' but for this test code I need to access the function that XField hides
                Output::send(STR("\t\t\tstatic_obj: {} ({})\n"), (void*)type_ptr, type_ptr->get_fname().to_string().c_str());

                if (UObject* owner = children->get_owner(); owner)
                {
                    Output::send(STR("\t\t\tOwner: {} ({})\n"), (void*)owner, owner->get_fname().to_string().c_str());
                }
                else
                {
                    Output::send(STR("\t\t\tOwner: None\n"));
                }

                Output::send(STR("\t\t\tOffset_Internal: {:#x}\n"), static_cast<XProperty*>(children)->get_offset_for_internal());

                Output::send(STR("\t\t}\n"));

                ++num_props_dumped;
            }

            if (num_props_dumped > 0)
            {
                Output::send(STR("\t\t...\n"));
            }
        };

        auto dump_object = [&](UObject* obj) {
            if (!obj)
            {
                Output::send(STR("Tried dumping nullptr obj\n"));
                return;
            }

            FName name = obj->get_fname();
            UClass* uclass = obj->get_uclass();

            XProperty* type_ptr = std::bit_cast<XProperty*>(obj->static_class());

            Output::send(STR("{} {} {{\n"), (void*)obj, obj->get_full_name());
            Output::send(STR("\tstatic_obj: {} ({})\n"), (void*)type_ptr, type_ptr->get_fname().to_string().c_str());
            Output::send(STR("\tinternal_index: {:#x}\n"), obj->get_internal_index());
            Output::send(STR("\tuclass: {} {}\n"), (void*)uclass, uclass->get_full_name());
            Output::send(STR("\tfname.comparison_index: {:#x}\n"), name.get_comparison_index());
            Output::send(STR("\tfname.number: {:#x}\n"), name.get_number());
            Output::send(STR("\tfname.to_string(): {}\n"), name.to_string().c_str());
            Output::send(STR("\touter: {} {}\n"), (void*)obj->get_outer(), obj->get_outer()->get_full_name());
            Output::send(STR("\ttype.as_string(): {}\n"), obj->get_type().as_string().c_str());
            Output::send(STR("\tget_name(): {}\n"), obj->get_name().c_str());
            dump_obj_flags(obj);
            Output::send(STR("\tis transient: {}\n"), obj->has_any_flag(EObjectFlags::RF_Transient) ? STR("yes") : STR("no"));
            Output::send(STR("\twas loaded: {}\n"), obj->has_any_flag(EObjectFlags::RF_WasLoaded) ? STR("yes") : STR("no"));
            Output::send(STR("\tis public or standalone: {}\n"), obj->has_any_flag(static_cast<EObjectFlags>(EObjectFlags::RF_Public | EObjectFlags::RF_Standalone)) ? STR("yes") : STR("no"));
            Output::send(STR("\tis function: {}\n"), obj->is_function() ? STR("yes") : STR("no"));

            if (obj->is_function())
            {
                // Casting to UClass because UFunction does not exist yet but I want access to some UStruct stuff
                // This is dangerous, don't do this in actual code
                Output::send(STR("\tproperties_size: {:#x}\n"), ((UClass*)obj)->get_properties_size());
                Output::send(STR("\tmin_alignment: {:#x}\n"), ((UClass*)obj)->get_min_alignment());
            }

            Output::send(STR("\tProperties: {\n"));

            if (obj->is_class())
            {
                UClass* obj_as_class = reinterpret_cast<UClass*>(obj);
                dump_properties(obj_as_class);
            }

            dump_properties(uclass);

            Output::send(STR("\t}\n"));

            Output::send(STR("}\n"));
        };

        UObject* obj = UObjectGlobals::static_find_object(nullptr, nullptr, L"/Script/CoreUObject.Object");
        dump_object(obj);

        UObject* engine_object1 = UObjectGlobals::static_find_object(nullptr, nullptr, L"/Engine/Transient.FGGameEngine_0");
        dump_object(engine_object1);

        UObject* engine_object2 = UObjectGlobals::static_find_object(nullptr, nullptr, L"/Engine/Transient.FGGameEngine_2147482616");
        dump_object(engine_object2);

        UObject* engine_object3 = UObjectGlobals::static_find_object(nullptr, nullptr, L"/Engine/Transient.TestableGameEngine_2147482618");
        dump_object(engine_object3);

        UObject* engine_object4 = UObjectGlobals::static_find_object(nullptr, nullptr, L"/Engine/Transient.GameEngine_2147482618");
        dump_object(engine_object4);

        UObject* engine_object5 = UObjectGlobals::static_find_object(nullptr, nullptr, L"/Engine/Transient.GameEngine_0");
        dump_object(engine_object5);

        UObject* obj4 = UObjectGlobals::static_find_object(nullptr, nullptr, L"/Script/CoreUObject.Default__Function");
        dump_object(obj4);

        UObject* obj5 = UObjectGlobals::static_find_object(nullptr, nullptr, L"/Script/CoreUObject.Object:ExecuteUbergraph");
        dump_object(obj5);

        UStruct* obj6 = UObjectGlobals::static_find_object<UStruct*>(nullptr, nullptr, L"/Script/Engine.Actor");
        dump_object(obj6);

        // Property Tests -> START
        // For game: Satisfactory CL#151024 (UE4.25.3.0)
        UObject* default_actor = UObjectGlobals::static_find_object(nullptr, nullptr, L"/Script/Engine.Default__Actor");
        dump_object(default_actor);

        auto test_bool_prop = [](UObject* obj, const wchar_t* prop_name, bool expected_val = false, bool do_test = false) -> bool {
            void* addr = obj->get_property_data_ptr(prop_name);
            if (!addr)
            {
                Output::send(STR("[ERROR] Property '{}' not found in UObject '{}'\n"), fmt(STR("%S"), prop_name), (void*)obj);
                return false;
            }
            else
            {
                bool val = obj->get_property<bool>(prop_name);
                Output::send(STR("{} (val: {} | expected: {}) <- {}\n"), addr, val ? STR("true") : STR("false"), expected_val ? STR("true") : STR("false"), fmt(L"%s", prop_name));
                if (do_test) { assert_test(val == expected_val, fmt(STR("BoolProperty (bitfield -> %s)"), prop_name).c_str())}
                return val;
            }
        };

        if (default_actor)
        {
            uint8_t* remote_role_val = default_actor->get_property<uint8_t>(L"RemoteRole");
            Output::send(STR("remote_role: {} (addr: {})\n"), *remote_role_val, (void*)remote_role_val);

            // Bitfield #1
            Output::send(STR("Dumping bitfield #1\n"));
            test_bool_prop(default_actor, L"bNetTemporary");
            test_bool_prop(default_actor, L"bNetStartup");
            test_bool_prop(default_actor, L"bOnlyRelevantToOwner");
            test_bool_prop(default_actor, L"bAlwaysRelevant");
            test_bool_prop(default_actor, L"bReplicateMovement");
            test_bool_prop(default_actor, L"bHidden");
            test_bool_prop(default_actor, L"bTearOff");
            test_bool_prop(default_actor, L"bExchangedRoles");

            // Bitfield #2
            Output::send(STR("Dumping bitfield #2\n"));
            test_bool_prop(default_actor, L"bNetLoadOnClient");
            test_bool_prop(default_actor, L"bNetUseOwnerRelevancy");
            test_bool_prop(default_actor, L"bRelevantForNetworkReplays");
            test_bool_prop(default_actor, L"bRelevantForLevelBounds");
            test_bool_prop(default_actor, L"bReplayRewindable");
            test_bool_prop(default_actor, L"bAllowTickBeforeBeginPlay");
            test_bool_prop(default_actor, L"bAutoDestroyWhenFinished");
        }

        // Is this how reading/writing values from/to UObjects should work ?
        if (engine_object1)
        {
            auto* int_val = engine_object1->get_property<int32_t>(L"ScreenSaverInhibitorSemaphore");
            auto* float_val = engine_object1->get_property<float>(L"ServerFlushLogInterval");

            Output::send(STR("int_val (#1): {} (addr: {})\n"), *int_val, (void*)int_val);
            Output::send(STR("float_val (#1): {} (addr: {})\n"), *float_val, (void*)float_val);
            //engine_obj->write<int32_t>("ScreenSaverInhibitorSemaphore", 1);
        }
        else
        {
            Output::send(STR("no engine object #1\n"));
        }

        if (engine_object2)
        {
            auto* int_val = engine_object2->get_property<int32_t>(L"ScreenSaverInhibitorSemaphore");
            auto* float_val = engine_object2->get_property<float>(L"ServerFlushLogInterval");

            Output::send(STR("int_val (#1): {} (addr: {})\n"), *int_val, (void*)int_val);
            Output::send(STR("float_val (#1): {} (addr: {})\n"), *float_val, (void*)float_val);
            //engine_obj->write<int32_t>("ScreenSaverInhibitorSemaphore", 1);

            test_bool_prop(engine_object2, L"bRenderLightMapDensityGrayscale");
            test_bool_prop(engine_object2, L"bHardwareSurveyEnabled");
            test_bool_prop(engine_object2, L"bSubtitlesEnabled");
            test_bool_prop(engine_object2, L"bSubtitlesForcedOff");
            test_bool_prop(engine_object2, L"bCanBlueprintsTickByDefault");
            test_bool_prop(engine_object2, L"bOptimizeAnimBlueprintMemberVariableAccess");
            test_bool_prop(engine_object2, L"bAllowMultiThreadedAnimationUpdate");
            test_bool_prop(engine_object2, L"bEnableEditorPSysRealtimeLOD");
            test_bool_prop(engine_object2, L"bSmoothFrameRate");
            test_bool_prop(engine_object2, L"bUseFixedFrameRate");

            UObject** game_viewport = engine_object2->get_property<UObject>(L"GameViewport");
            Output::send(STR("GameViewport: {}\n"), (void*)*game_viewport);

            UObject* asset_manager = *engine_object2->get_property<UObject>(L"AssetManager");
            Output::send(STR("asset_manager: {}\n"), (void*)asset_manager);

            TArray<UObject*> object_reference_list = asset_manager->get_property<TArray<UObject*>>(L"ObjectReferenceList");
            Output::send(STR("Manual access -> START\n"));
            if (object_reference_list.get_array_num() >= 4)
            {
                Output::send(STR("object_reference_list[0]: {} (&:{})\n"), (void*)*object_reference_list[0], (void*)object_reference_list[0]);
                Output::send(STR("object_reference_list[1]: {} (&:{})\n"), (void*)*object_reference_list[1], (void*)object_reference_list[1]);
                Output::send(STR("object_reference_list[2]: {} (&:{})\n"), (void*)*object_reference_list[2], (void*)object_reference_list[2]);
                Output::send(STR("object_reference_list[3]: {} (&:{})\n"), (void*)*object_reference_list[3], (void*)object_reference_list[3]);
            }
            Output::send(STR("Manual access -> END\n"));

            Output::send(STR("Loop access (with both elem & index passed) -> START\n"));
            object_reference_list.for_each([](UObject** elem, size_t index) {
                Output::send(STR("elem: {} (&elem: {}) | index: {}\n"), (void*)*elem, (void*)elem, index);
                return LoopAction::Continue;
            });
            Output::send(STR("Loop access (with both elem & index passed) -> END\n"));

            Output::send(STR("Loop access (with only elem passed) -> START\n"));
            object_reference_list.for_each([](UObject** elem) {
                Output::send(STR("elem: {} (&elem: {})\n"), (void*)*elem, (void*)elem);
                return LoopAction::Continue;
            });
            Output::send(STR("Loop access (with only elem passed) -> END\n"));
        }
        else
        {
            Output::send(STR("no engine object #2\n"));
        }

        if (engine_object3)
        {
            // TODO: Turn all of this into proper tests

            // Numerical Tests
            // TODO: uint8/byte, uint16, uint32, uint64
            int8_t* int8_val = engine_object3->get_property<int8_t>(L"m_testable_int8");
            Output::send(STR("{} (val: {} | expected: {}) <- int8\n"), (void*)int8_val, *int8_val, std::numeric_limits<int8_t>::max());
            assert_test(*int8_val == std::numeric_limits<int8_t>::max(), STR("Int8Property"))

            int16_t* int16_val = engine_object3->get_property<int16_t>(L"m_testable_int16");
            Output::send(STR("{} (val: {} | expected: {}) <- int16\n"), (void*)int16_val, *int16_val, std::numeric_limits<int16_t>::max());
            assert_test(*int16_val == std::numeric_limits<int16_t>::max(), STR("Int16Property"))

            int32_t* int32_val = engine_object3->get_property<int32_t>(L"m_testable_int32");
            Output::send(STR("{} (val: {} | expected: {}) <- int32\n"), (void*)int32_val, *int32_val, std::numeric_limits<int32_t>::max());
            assert_test(*int32_val == std::numeric_limits<int32_t>::max(), STR("IntProperty"))

            int64_t* int64_val = engine_object3->get_property<int64_t>(L"m_testable_int64");
            Output::send(STR("{} (val: {} | expected: {}) <- int64\n"), (void*)int64_val, *int64_val, std::numeric_limits<int64_t>::max());
            assert_test(*int64_val == std::numeric_limits<int64_t>::max(), STR("Int64Property"))

            // uint8 / byte
            uint8_t* byte_val = engine_object3->get_property<uint8_t>(L"m_testable_uint8");
            Output::send(STR("{} (val: {} | expected: {}) <- byte\n"), (void*)byte_val, *byte_val, std::numeric_limits<uint8_t>::max());
            assert_test(*byte_val == std::numeric_limits<uint8_t>::max(), STR("ByteProperty"))

            float* float_val = engine_object3->get_property<float>(L"m_testable_float");
            Output::send(STR("{} (val: {} | expected: {}) <- float\n"), (void*)float_val, *float_val, 99.4f);
            assert_test(*float_val == 99.4f, STR("FloatProperty"))

            // Bool Tests
            // C++ bool
            void* bool_addr = engine_object3->get_property_data_ptr(L"m_testable_bool");
            bool bool_val = engine_object3->get_property<bool>(L"m_testable_bool");
            Output::send(STR("{} (val: {} | expected: {}) <- c++ bool\n"), bool_addr, bool_val ? STR("true") : STR("false"), STR("true"));
            assert_test(bool_val == true, STR("BoolProperty (C++ bool)"))

            // Bitfield
            test_bool_prop(engine_object3, L"m_testable_bool_bit1", true, true);
            test_bool_prop(engine_object3, L"m_testable_bool_bit2", true, true);
            test_bool_prop(engine_object3, L"m_testable_bool_bit3", true, true);
            test_bool_prop(engine_object3, L"m_testable_bool_bit4", false, true);
            test_bool_prop(engine_object3, L"m_testable_bool_bit5", false, true);
            test_bool_prop(engine_object3, L"m_testable_bool_bit6", true, true);
            test_bool_prop(engine_object3, L"m_testable_bool_bit7", true, true);
            test_bool_prop(engine_object3, L"m_testable_bool_bit8", true, true);

            // Object Tests
            UObject** obj_val = engine_object3->get_property<UObject>(L"m_testable_object");
            Output::send(STR("{} (val: {} | expected: {}) <- ObjectProperty\n"), (void*)obj_val, (void*)*obj_val, (void*)engine_object3);
            assert_test(*obj_val == engine_object3, STR("ObjectProperty"))

            // Uncomment when WeakObjectProperty is supported
            /*
            void* weak_obj_addr = engine_object3->get_property_data_ptr(L"m_testable_weakobject");
            UObject* weak_obj_val = *engine_object3->get_property<XObjectProperty>(L"m_testable_weakobject");
            printf_s("%p (val: %p | expected: %p) <- ObjectProperty\n", weak_obj_addr, weak_obj_val, engine_object3);
            */

            // Array Tests
            void* array_addr = engine_object3->get_property_data_ptr(L"m_testable_array_of_int");
            TArray<int32_t> array_val = engine_object3->get_property<TArray<int32_t>>(L"m_testable_array_of_int");
            array_val.for_each([&](const int32_t* elem, auto index) {
                Output::send(STR("{}[{}] -> {} (val: {} | expected: {}) <- ArrayProperty <- TArray<int32>\n"), array_addr, index, (void*)elem, *elem, index + 1);
                assert_test(*elem == index + 1, STR("ArrayProperty <- TArray<int32_t>"))

                return LoopAction::Continue;
            });

            void* array_of_obj_addr = engine_object3->get_property_data_ptr(L"m_testable_array_of_uobject");
            TArray<UObject*> array_of_obj = engine_object3->get_property<TArray<UObject*>>(L"m_testable_array_of_uobject");
            array_of_obj.for_each([&](UObject** elem, auto index) {
                Output::send(STR("{}[{}] -> {} (val: {} | expected: {}) <- ArrayProperty <- TArray<UObject*>\n"), array_of_obj_addr, index, (void*)elem, (void*)*elem, (void*)engine_object3);
                assert_test(*elem == engine_object3, STR("ArrayProperty <- TArray<UObject*>"))

                return LoopAction::Continue;
            });

            // Struct Tests
            {
                XStruct struct_of_int = engine_object3->get_property<XStructProperty>(L"m_testable_struct_of_int");
                int32_t* m_int_one = struct_of_int.get_property<int32_t>(L"m_int_one");
                assert_test(*m_int_one == 1, STR("StructProperty <- int32_t (m_int_one)"))
                int32_t* m_int_two = struct_of_int.get_property<int32_t>(L"m_int_two");
                assert_test(*m_int_two == 2, STR("StructProperty <- int32_t (m_int_two)"))
                int32_t* m_int_three = struct_of_int.get_property<int32_t>(L"m_int_three");
                assert_test(*m_int_three == 3, STR("StructProperty <- int32_t (m_int_three)"))
                Output::send(STR("{}[m_int_one] -> {} (val: {} | expected: {}) <- StructProperty <- int32_t\n"), (void*)m_int_one, (void*)m_int_one, *m_int_one, 1);
                Output::send(STR("{}[m_int_two] -> {} (val: {} | expected: {}) <- StructProperty <- int32_t\n"), (void*)m_int_one, (void*)m_int_two, *m_int_two, 2);
                Output::send(STR("{}[m_int_three] -> {} (val: {} | expected: {}) <- StructProperty <- int32_t\n"), (void*)m_int_one, (void*)m_int_three, *m_int_three, 3);
            }

            {
                XStruct struct_of_array_of_int = engine_object3->get_property<XStructProperty>(L"m_testable_struct_of_array_of_int");
                TArray<int32_t> m_testable_array_of_int = struct_of_array_of_int.get_value<TArray<int32_t>>(L"m_testable_array_of_int");
                m_testable_array_of_int.for_each([&](int32_t* elem, auto index) {
                    Output::send(STR("{}[{}] -> {} (val: {} | expected: {}) <- StructProperty <- ArrayProperty <- TArray<int32_t>\n"), (void*)m_testable_array_of_int.get_data_ptr(), index, (void*)elem, *elem, index + 1);
                    assert_test(*elem == index + 1, STR("StructProperty < ArrayProperty <- TArray<int32_t>"))
                    return LoopAction::Continue;
                });
            }

            printf_s("testing array of struct of int\n");
            TArray<XStruct> array_of_struct_of_int = engine_object3->get_property<TArray<XStruct>>(L"m_testable_array_of_struct_of_int");
            printf_s("array_of_struct_of_int retrieved\n");
            printf_s("array_of_struct_of_int.ptr: %p\n", array_of_struct_of_int.get_data_ptr());
            printf_s("array_of_struct_of_int.num: %d\n", array_of_struct_of_int.get_array_num());
            printf_s("array_of_struct_of_int.max: %d\n", array_of_struct_of_int.get_array_max());
            array_of_struct_of_int.for_each([&](XStruct* elem, auto index) {
                printf_s("inside for_each\n");
                // TODO: Improve this test, I think there is too much repetition
                int32_t* val_one = elem->get_property<int32_t>(L"m_int_one");
                int32_t* val_two = elem->get_property<int32_t>(L"m_int_two");
                int32_t* val_three = elem->get_property<int32_t>(L"m_int_three");

                if (index == 0)
                {
                    Output::send(STR("{}[{}] -> {} (val: {} | expected: 1) <- ArrayProperty <- StructProperty (int32_t)>\n"), (void*)array_of_struct_of_int.get_data_ptr(), index, (void*)val_one, *val_one);
                    assert_test(*val_one == 1, fmt(STR("ArrayProperty < StructProperty < int32_t (index: %d)"), index).c_str())

                    Output::send(STR("{}[{}] -> {} (val: {} | expected: 2) <- ArrayProperty <- StructProperty (int32_t)>\n"), (void*)array_of_struct_of_int.get_data_ptr(), index, (void*)val_two, *val_two);
                    assert_test(*val_two == 2, fmt(STR("ArrayProperty < StructProperty < int32_t (index: %d)"), index).c_str())

                    Output::send(STR("{}[{}] -> {} (val: {} | expected: 3) <- ArrayProperty <- StructProperty (int32_t)>\n"), (void*)array_of_struct_of_int.get_data_ptr(), index, (void*)val_three, *val_three);
                    assert_test(*val_three == 3, fmt(STR("ArrayProperty < StructProperty < int32_t (index: %d)"), index).c_str())
                }
                else if (index == 1)
                {
                    Output::send(STR("{}[{}] -> {} (val: {} | expected: 4) <- ArrayProperty <- StructProperty (int32_t)>\n"), (void*)array_of_struct_of_int.get_data_ptr(), index, (void*)val_one, *val_one);
                    assert_test(*val_one == 4, fmt(STR("ArrayProperty < StructProperty < int32_t (index: %d)"), index).c_str())

                    Output::send(STR("{}[{}] -> {} (val: {} | expected: 5) <- ArrayProperty <- StructProperty (int32_t)>\n"), (void*)array_of_struct_of_int.get_data_ptr(), index, (void*)val_two, *val_two);
                    assert_test(*val_two == 5, fmt(STR("ArrayProperty < StructProperty < int32_t (index: %d)"), index).c_str())

                    Output::send(STR("{}[{}] -> {} (val: {} | expected: 6) <- ArrayProperty <- StructProperty (int32_t)>\n"), (void*)array_of_struct_of_int.get_data_ptr(), index, (void*)val_three, *val_three);
                    assert_test(*val_three == 6, fmt(STR("ArrayProperty < StructProperty < int32_t (index: %d)"), index).c_str())
                }
                else if (index == 2)
                {
                    Output::send(STR("{}[{}] -> {} (val: {} | expected: 7) <- ArrayProperty <- StructProperty (int32_t)>\n"), (void*)array_of_struct_of_int.get_data_ptr(), index, (void*)val_one, *val_one);
                    assert_test(*val_one == 7, fmt(STR("ArrayProperty < StructProperty < int32_t (index: %d)"), index).c_str())

                    Output::send(STR("{}[{}] -> {} (val: {} | expected: 8) <- ArrayProperty <- StructProperty (int32_t)>\n"), (void*)array_of_struct_of_int.get_data_ptr(), index, (void*)val_two, *val_two);
                    assert_test(*val_two == 8, fmt(STR("ArrayProperty < StructProperty < int32_t (index: %d)"), index).c_str())

                    Output::send(STR("{}[{}] -> {} (val: {} | expected: 9) <- ArrayProperty <- StructProperty (int32_t)>\n"), (void*)array_of_struct_of_int.get_data_ptr(), index, (void*)val_three, *val_three);
                    assert_test(*val_three == 9, fmt(STR("ArrayProperty < StructProperty < int32_t (index: %d)"), index).c_str())
                }
                return LoopAction::Continue;
            });

            // WeakObject Test
            auto do_weak_object_test = [&engine_object3](UObject* expected_value, std::wstring_view expected_value_name) -> FWeakObjectPtr* {
                FWeakObjectPtr* weak_object = engine_object3->get_property<FWeakObjectPtr>(L"m_testable_weakobject");
                assert_test(weak_object, STR("FWeakObjectPtr instance (TestableGameEngine:m_testable_weakobject) was nullptr"))

                UObject* real_object = weak_object->get();
                Output::send(STR("FWeakObjectPtr: {} | Real Object: {} - {}\n"), (void*)weak_object, (void*)real_object, real_object->get_full_name());

                assert_test(real_object == expected_value, std::format(L"Expected 'real_object' to be {}({}) but was {}", expected_value_name, (void*)expected_value, (void*)real_object).c_str())

                return weak_object;
            };

            FWeakObjectPtr* weak_object = do_weak_object_test(engine_object3, L"TestableGameEngine");

            *weak_object = nullptr;
            weak_object = do_weak_object_test(nullptr, L"NULLPTR");

            *weak_object = engine_object3;
            do_weak_object_test(engine_object3, L"TestableGameEngine");

            // Custom Property Test, trivial underlying type
            UObject* object_for_custom_type_test = engine_object3;
            assert_test(object_for_custom_type_test, STR("Expected 'object_for_custom_type_test' to be non-nullptr but it was nullptr"))
            // Custom data is pointing to 'TestableGameEngine:m_testable_int8'
            Unreal::CustomProperty custom_int_property{0xE30};
            int8_t* custom_data = object_for_custom_type_test->get_property<int8_t>(L"Int8WithNoMetadata", &custom_int_property);
            assert_test(custom_data, STR("Expected 'custom_data' to be non-nullptr but it was nullptr"))
            Output::send(STR("Custom data (fully custom) -> Underlying type: int8_t, trivial: yes\n"));
            Output::send(STR("custom_data (m_testable_int8): {}\n"), *custom_data);
            assert_test(*custom_data == std::numeric_limits<int8_t>::max(), std::format(L"Expected '*custom_data' to be equal to '{}' but it was '{}'", std::numeric_limits<int8_t>::max(), *custom_data).c_str())

            // Custom Property Test, non-trivial underlying type
            /**/
            {
                // Custom data is pointing to 'TestableGameEngine:m_testable_array_of_int'
                Unreal::CustomProperty custom_array_of_int_property{0xFA8};
                TArray<int32_t> custom_array = engine_object3->get_property<TArray<int32_t>>(L"ArrayOfIntWithNoMetadata", &custom_array_of_int_property);
                Output::send(STR("Custom data (fully custom) -> Underlying type: TArray<int32_t>, trivial: yes\n"));
                custom_array.for_each([&](const int32_t* elem, auto index) {
                    Output::send(STR("[{}] -> {} (val: {} | expected: {}) <- ArrayProperty(Custom) <- TArray<int32>\n"), index, (void*)elem, *elem, index + 1);
                    assert_test(*elem == index + 1, STR("ArrayProperty <- TArray<int32_t>"))

                    return LoopAction::Continue;
                });
            }
            //*/

            {
                // Custom data is pointing to 'TestableGameEngine:m_testable_array_of_struct_of_int'
                Unreal::UScriptStruct* script_struct_for_custom_struct_property = UObjectGlobals::static_find_object<Unreal::UScriptStruct*>(nullptr, nullptr, L"/Script/UE4SS_Main426.TestableStructOfInt");
                assert_test(script_struct_for_custom_struct_property, "script_struct_for_custom_struct_property was nullptr")
                std::unique_ptr<Unreal::CustomProperty> test_custom_struct_property = CustomStructProperty::construct(0x0, script_struct_for_custom_struct_property);
                std::unique_ptr<Unreal::CustomProperty> test_custom_array_property = CustomArrayProperty::construct(0xFD8, test_custom_struct_property.get());
                Output::send(STR("test_custom_array_property: {}\n"), (void*)test_custom_array_property.get());
                /**/
                TArray<XStruct> custom_struct = engine_object3->get_property<TArray<XStruct>>(L"ArrayOfStructWithNoMetadata", test_custom_array_property.get());
                Output::send(STR("Custom data (partially custom) -> Underlying type: TArray<XStruct>, trivial: no\n"));
                custom_struct.for_each([&](XStruct* elem, auto index) {
                    // TODO: Improve this test, I think there is too much repetition
                    int32_t* val_one = elem->get_property<int32_t>(L"m_int_one");
                    int32_t* val_two = elem->get_property<int32_t>(L"m_int_two");
                    int32_t* val_three = elem->get_property<int32_t>(L"m_int_three");

                    if (index == 0)
                    {
                        Output::send(STR("{}[{}] -> {} (val: {} | expected: 1) <- ArrayProperty(Custom) <- StructProperty (int32_t)>\n"), (void*)custom_struct.get_data_ptr(), index, (void*)val_one, *val_one);
                        assert_test(*val_one == 1, fmt(STR("ArrayProperty(Custom) < StructProperty < int32_t (index: %d)"), index).c_str())

                        Output::send(STR("{}[{}] -> {} (val: {} | expected: 2) <- ArrayProperty(Custom) <- StructProperty (int32_t)>\n"), (void*)custom_struct.get_data_ptr(), index, (void*)val_two, *val_two);
                        assert_test(*val_two == 2, fmt(STR("ArrayProperty(Custom) < StructProperty < int32_t (index: %d)"), index).c_str())

                        Output::send(STR("{}[{}] -> {} (val: {} | expected: 3) <- ArrayProperty(Custom) <- StructProperty (int32_t)>\n"), (void*)custom_struct.get_data_ptr(), index, (void*)val_three, *val_three);
                        assert_test(*val_three == 3, fmt(STR("ArrayProperty(Custom) < StructProperty < int32_t (index: %d)"), index).c_str())
                    }
                    else if (index == 1)
                    {
                        Output::send(STR("{}[{}] -> {} (val: {} | expected: 4) <- ArrayProperty(Custom) <- StructProperty (int32_t)>\n"), (void*)custom_struct.get_data_ptr(), index, (void*)val_one, *val_one);
                        assert_test(*val_one == 4, fmt(STR("ArrayProperty(Custom) < StructProperty < int32_t (index: %d)"), index).c_str())

                        Output::send(STR("{}[{}] -> {} (val: {} | expected: 5) <- ArrayProperty(Custom) <- StructProperty (int32_t)>\n"), (void*)custom_struct.get_data_ptr(), index, (void*)val_two, *val_two);
                        assert_test(*val_two == 5, fmt(STR("ArrayProperty(Custom) < StructProperty < int32_t (index: %d)"), index).c_str())

                        Output::send(STR("{}[{}] -> {} (val: {} | expected: 6) <- ArrayProperty(Custom) <- StructProperty (int32_t)>\n"), (void*)custom_struct.get_data_ptr(), index, (void*)val_three, *val_three);
                        assert_test(*val_three == 6, fmt(STR("ArrayProperty(Custom) < StructProperty < int32_t (index: %d)"), index).c_str())
                    }
                    else if (index == 2)
                    {
                        Output::send(STR("{}[{}] -> {} (val: {} | expected: 7) <- ArrayProperty(Custom) <- StructProperty (int32_t)>\n"), (void*)custom_struct.get_data_ptr(), index, (void*)val_one, *val_one);
                        assert_test(*val_one == 7, fmt(STR("ArrayProperty(Custom) < StructProperty < int32_t (index: %d)"), index).c_str())

                        Output::send(STR("{}[{}] -> {} (val: {} | expected: 8) <- ArrayProperty(Custom) <- StructProperty (int32_t)>\n"), (void*)custom_struct.get_data_ptr(), index, (void*)val_two, *val_two);
                        assert_test(*val_two == 8, fmt(STR("ArrayProperty(Custom) < StructProperty < int32_t (index: %d)"), index).c_str())

                        Output::send(STR("{}[{}] -> {} (val: {} | expected: 9) <- ArrayProperty(Custom) <- StructProperty (int32_t)>\n"), (void*)custom_struct.get_data_ptr(), index, (void*)val_three, *val_three);
                        assert_test(*val_three == 9, fmt(STR("ArrayProperty(Custom) < StructProperty < int32_t (index: %d)"), index).c_str())
                    }
                    return LoopAction::Continue;
                });
            }
            //*/

            {
                // Custom data is pointing to 'TestableGameEngine:m_testable_struct_of_int'
                Unreal::UScriptStruct* script_struct_for_custom_struct_property = UObjectGlobals::static_find_object<Unreal::UScriptStruct*>(nullptr, nullptr, L"/Script/UE4SS_Main426.TestableStructOfInt");
                assert_test(script_struct_for_custom_struct_property, "script_struct_for_custom_struct_property was nullptr")
                std::unique_ptr<Unreal::CustomProperty> test_custom_struct_property = CustomStructProperty::construct(0xF98, script_struct_for_custom_struct_property);
                Output::send(STR("test_custom_struct_property: {}\n"), (void*)test_custom_struct_property.get());
                XStruct custom_struct_of_int = engine_object3->get_property<XStructProperty>(L"StructOfIntWithNoMetadata", test_custom_struct_property.get());
                Output::send(STR("Custom data (partially custom) -> Underlying type: XStruct, trivial: no\n"));

                /**/
                int32_t* m_int_one = custom_struct_of_int.get_property<int32_t>(L"m_int_one");
                assert_test(*m_int_one == 1, STR("StructProperty(Custom) <- int32_t (m_int_one)"))
                int32_t* m_int_two = custom_struct_of_int.get_property<int32_t>(L"m_int_two");
                assert_test(*m_int_two == 2, STR("StructProperty(Custom) <- int32_t (m_int_two)"))
                int32_t* m_int_three = custom_struct_of_int.get_property<int32_t>(L"m_int_three");
                assert_test(*m_int_three == 3, STR("StructProperty(Custom) <- int32_t (m_int_three)"))
                Output::send(STR("{}[m_int_one] -> {} (val: {} | expected: {}) <- StructProperty(Custom) <- int32_t\n"), (void*)m_int_one, (void*)m_int_one, *m_int_one, 1);
                Output::send(STR("{}[m_int_two] -> {} (val: {} | expected: {}) <- StructProperty(Custom) <- int32_t\n"), (void*)m_int_one, (void*)m_int_two, *m_int_two, 2);
                Output::send(STR("{}[m_int_three] -> {} (val: {} | expected: {}) <- StructProperty(Custom) <- int32_t\n"), (void*)m_int_one, (void*)m_int_three, *m_int_three, 3);
                //*/
            }

            {
                // Custom data is pointing to 'TestableGameEngine:m_testable_struct_of_array_of_int'
                std::unique_ptr<Unreal::CustomProperty> test_custom_struct_property = CustomStructProperty::construct(0xFC8, nullptr);
                XStruct struct_of_array_of_int = engine_object3->get_property<XStructProperty>(L"StructOfArrayOfIntWithNoMetadata", test_custom_struct_property.get());
                Unreal::CustomProperty custom_array_of_int_property{0x0};
                TArray<int32_t> custom_array = struct_of_array_of_int.get_value<TArray<int32_t>>(L"ArrayOfIntWithNoMetadata", &custom_array_of_int_property);

                Output::send(STR("Custom data (fully custom) -> Underlying type: XStruct -> TArray<int>, trivial: no\n"));
                assert_test(custom_array.get_array_num() > 0, STR("custom_array was empty"))
                custom_array.for_each([&](int32_t* elem, auto index) {
                    Output::send(STR("{}[{}] -> {} (val: {} | expected: {}) <- StructProperty(Custom) <- ArrayProperty(Custom) <- TArray<int32_t>\n"), (void*)custom_array.get_data_ptr(), index, (void*)elem, *elem, index + 1);
                    assert_test(*elem == index + 1, STR("StructProperty(Custom) < ArrayProperty(Custom) <- TArray<int32_t>"))
                    return LoopAction::Continue;
                });
            }

            {
                // Custom data is pointing to 'TestableGameEngine:m_testable_struct_of_array_of_int'
                Unreal::UScriptStruct* script_struct_for_custom_struct_property = UObjectGlobals::static_find_object<Unreal::UScriptStruct*>(nullptr, nullptr, L"/Script/UE4SS_Main426.TestableStructOfArrayOfInt");
                assert_test(script_struct_for_custom_struct_property, "script_struct_for_custom_struct_property was nullptr")
                std::unique_ptr<Unreal::CustomProperty> test_custom_struct_property = CustomStructProperty::construct(0xFC8, script_struct_for_custom_struct_property);
                XStruct struct_of_array_of_int = engine_object3->get_property<XStructProperty>(L"StructOfArrayOfIntWithNoMetadata", test_custom_struct_property.get());
                TArray<int32_t> custom_array = struct_of_array_of_int.get_value<TArray<int32_t>>(L"m_testable_array_of_int");

                Output::send(STR("Custom data (partially custom) -> Underlying type: XStruct -> TArray<int>, trivial: no\n"));
                assert_test(custom_array.get_array_num() > 0, STR("custom_array was empty"))
                custom_array.for_each([&](int32_t* elem, auto index) {
                    Output::send(STR("{}[{}] -> {} (val: {} | expected: {}) <- StructProperty(Custom) <- ArrayProperty <- TArray<int32_t>\n"), (void*)custom_array.get_data_ptr(), index, (void*)elem, *elem, index + 1);
                    assert_test(*elem == index + 1, STR("StructProperty(Custom) < ArrayProperty <- TArray<int32_t>"))
                    return LoopAction::Continue;
                });
            }
        }
        else
        {
            Output::send(STR("no engine object #3\n"));
        }

        if (engine_object4)
        {
            auto* int_val = engine_object4->get_property<int32_t>(L"ScreenSaverInhibitorSemaphore");
            auto* float_val = engine_object4->get_property<float>(L"ServerFlushLogInterval");

            Output::send(STR("int_val (#3): {} (address: {})\n"), *int_val, (void*)int_val);
            Output::send(STR("float_val (#3): {} (address: {})\n"), *float_val, (void*)float_val);
            //engine_obj->write<int32_t>("ScreenSaverInhibitorSemaphore", 1);
        }
        else
        {
            Output::send(STR("no engine object #4\n"));
        }

        if (engine_object5)
        {
            auto* int_val = engine_object5->get_property<int32_t>(L"ScreenSaverInhibitorSemaphore");
            auto* float_val = engine_object5->get_property<float>(L"ServerFlushLogInterval");

            Output::send(STR("int_val (#4): {} (address: {})\n"), *int_val, (void*)int_val);
            Output::send(STR("float_val (#4): {} (address: {})\n"), *float_val, (void*)float_val);
            //engine_obj->write<int32_t>("ScreenSaverInhibitorSemaphore", 1);
        }
        else
        {
            Output::send(STR("no engine object #5\n"));
        }
        // Property Tests -> END

        // OutputDevice example code
        /**/
        {
            // Setup
            // Set the default device which is never deallocated unless the default device is set again
            //Output::set_default_devices<Output::DebugConsoleDevice>();
            //Output::set_default_devices<Output::DebugConsoleDevice, Output::TestDevice>();

            // Simple output to all default devices
            Output::send(STR("Hello default devices\n"));
            Output::send(STR("Hello default devices with arg {}\n"), 5);
            Output::send(STR("Hello default device with more args {} {}\n"), 5, 6);

            // Output to the specified devices and keep all devices ready for more output until we either go out of scope or call close_devices()
            // Note that you don't have to specify any OutputDevice if Output::open_devices() was called first
            Output::Targets<Output::DebugConsoleDevice, Output::TestDevice> scoped_out;
            scoped_out.send(STR("Hello persistent devices #1\n"), Output::TestDevice::OptionalArgTest::ValueThree);
            scoped_out.send(STR("Hello persistent devices #2 - {}\n"), Output::TestDevice::OptionalArgTest::ValueTwo, 1);
            scoped_out.send(STR("Hello persistent devices #3 - {}\n"), Output::TestDevice::OptionalArgTest::ValueOne, 2);
            scoped_out.send(STR("Hello persistent devices #4 - {}\n"), Output::TestDevice::OptionalArgTest::ValueOne, 3);
            scoped_out.send(STR("Hello persistent devices #5 - {}\n"), Output::TestDevice::OptionalArgTest::ValueOne, 4.4f);
            scoped_out.send(STR("Hello persistent devices #6 - no args\n"), Output::TestDevice::OptionalArgTest::ValueOne);

            // Method #1 - Use a constexpr "string" type as a template param
            //static constexpr char my_filename[]{STR("D:\\test\\my_file.txt")};
            //Output::send<Output::FileDevice<my_filename>>(STR("Test FileOutputDevice - no args\n"));

            // Method #2 - Pass a runtime "string" to a function call
            // May require this output device to be used via the "Output::Targets" method
            using TestFileOutputDevice = Output::FileDevice;
            Output::Targets<TestFileOutputDevice, Output::DebugConsoleDevice> scoped_out2;
            auto& file_output_device = scoped_out2.get_device<TestFileOutputDevice>();
            file_output_device.set_file_name_and_path(STR("D:\\test\\test2\\test3\\test4\\my_file.ini"));
            scoped_out2.send(STR("Hello333 - no args\n"));
            scoped_out2.send(STR("Hello444 - no args\n"));
            scoped_out2.send(STR("Hello555 - {} {}\n"), Color::Red, 3, 4);

            // Method #3 - Identical to method #2 except with default devices
            //auto& file_device = Output::get_device<Output::FileDevice>();
            //file_output_device.set_file_name_and_path(STR("D:\\test\\my_file2.ini"));
            //scoped_out2.send(STR("Hello1 - no args\n"));
            //scoped_out2.send(STR("Hello2 - no args\n"));
            //scoped_out2.send(STR("Hello3 - {} {}\n"), 3, 4);
        }
        //*/

        UObject* player_controller = UObjectGlobals::static_find_object(nullptr, nullptr, L"/Game/FirstPersonCPP/Maps/FirstPersonExampleMap.FirstPersonExampleMap.PersistentLevel.PlayerController_2147482560");
        if (player_controller)
        {
            //dump_flags(player_controller->get_flags());
            Output::send(STR("PlayerController FName: {:#x} / {:#x} ({})\n"), player_controller->get_fname().get_number(), player_controller->get_fname().get_comparison_index(), player_controller->get_fname().to_string());
        }
        else
        {
            Output::send(STR("No PlayerController\n"));
        }

        FName player_controller_class{L"PlayerController"};
        UObject* player_controller_test_one = UObjectGlobals::find_first_of(player_controller_class);
        UObject* player_controller_test_two = UObjectGlobals::find_first_of(L"PlayerController");
        UObject* engine_test = UObjectGlobals::find_first_of(L"Engine");

        std::vector<UObject*> all_pawns_test;
        UObjectGlobals::find_all_of(L"Pawn", all_pawns_test);

        /**/
        if (player_controller_test_one)
        {
            Output::send(L"player_controller_test_one: {} ({})\n", (void*)player_controller_test_one, player_controller_test_one->get_full_name());
        }
        else
        {
            Output::send(STR("No PlayerController #1 found\n"));
        }
        //*/

        /**/
        if (player_controller_test_two)
        {
            Output::send(L"player_controller_test_one: {} ({})\n", (void*)player_controller_test_two, player_controller_test_two->get_full_name());
        }
        else
        {
            Output::send(STR("No PlayerController #2 found\n"));
        }
        //*/

        /**/
        if (engine_test)
        {
            Output::send(L"engine_test_one: {} ({})\n", (void*)engine_test, engine_test->get_full_name());
        }
        else
        {
            Output::send(STR("No Engine #1 found\n"));
        }
        //*/

        /**/
        if (!all_pawns_test.empty())
        {
            Output::send(STR("All Pawns:\n"));
            for (const auto& pawn : all_pawns_test)
            {
                Output::send(STR("{}\n"), pawn->get_full_name());
            }
        }
        else
        {
            Output::send(STR("No pawns found\n"));
        }
        //*/

        // StaticConstructObject_Internal test (creating console)
        // Commented out because I'm executing the same code in the Lua test script
        /*
        UObject* engine_for_console_creation = UObjectGlobals::find_first_of(L"Engine");
        UClass* console_class = static_cast<UClass*>(*engine_for_console_creation->get_property<UObject>(L"ConsoleClass"));
        UObject* viewport = *engine_for_console_creation->get_property<UObject>(L"GameViewport");

        if (!console_class || !viewport)
        {
            throw std::runtime_error{"Tried to create console but console class or viewport was nullptr"};
        }

        UObject* console = UObjectGlobals::static_construct_object(Unreal::FStaticConstructObjectParameters{
            .Class = console_class,
            .Outer = viewport,
            .Name = FName(L"None"),
            .SetFlags = Unreal::RF_NoFlags,
            .InternalSetFlags = Unreal::EInternalObjectFlags::None,
            .bCopyTransientsFromClassDefaults = false,
            .bAssumeTemplateIsArchetype = false,
            .Template = nullptr,
            .InstanceGraph = nullptr,
            .ExternalPackage = nullptr,
            .SubobjectOverrides = nullptr,
        });

        Output::send(STR("Console: {}\n"), (void*)console);

        UObject** viewport_console = viewport->get_property<UObject>(L"ViewportConsole");
        *viewport_console = console;
        //*/

        // Change the console key
        UObject* input_settings = UObjectGlobals::static_find_object(nullptr, nullptr, L"/Script/Engine.Default__InputSettings");
        if (!input_settings) { throw std::runtime_error{"Was unable to find '/Script/Engine.Default__InputSettings'"}; }

        TArray<FName> console_keys = input_settings->get_property<TArray<FName>>(L"ConsoleKeys");
        *console_keys[0] = FName(L"F12");
        console_keys.for_each([&](FName* elem, auto index) {
            Output::send(STR("ConsoleKey #{}: {} | 0x{:X}\n"), index, elem->to_string(), elem->get_comparison_index());
            return LoopAction::Continue;
        });

        // UFunction Hook Test -> START
        UFunction* can_jump_internal_func = UObjectGlobals::static_find_object<UFunction*>(nullptr, nullptr, L"/Script/Engine.Character:CanJumpInternal");
        if (!can_jump_internal_func) { throw std::runtime_error{"UFunction Hook Test failed, 'can_jump_internal_func' is nullptr!"}; }

        can_jump_internal_func->register_hook([](...) {
            // Pre-hook (use for param access & mutation)

            Output::send(STR("[C++] Jump hook #1\n"));
        }, []([[maybe_unused]]Unreal::UnrealScriptFunctionCallableContextParam context, ...) {
            // Post-hook (use for return value access & mutation)

            //context.set_return_value(true);
        });

        UFunction* set_movement_mode_func = UObjectGlobals::static_find_object<UFunction*>(nullptr, nullptr, L"/Script/Engine.CharacterMovementComponent:SetMovementMode");
        if (!set_movement_mode_func) { throw std::runtime_error{"UFunction Hook Test failed, 'set_movement_mode_func' is nullptr!"}; }

        set_movement_mode_func->register_hook([](Unreal::UnrealScriptFunctionCallableContextParam context, ...) {
            auto& params = context.get_params<UCharacterMovementComponent::SetMovementMode_Params>();
            Output::send(STR("[C++] SetMovementMode hook #1 - NewMovementMode: {} | NewCustomMode: {}\n"), params.NewMovementMode, params.NewCustomMode);
        }, [](...) {
        });

        set_movement_mode_func->register_hook([](...) {
            Output::send(STR("[C++] SetMovementMode hook #2\n"));
        }, [](...) {
        });
        // UFunction Hook Test -> END

        // UEnum Test -> START
        if (Unreal::Version::is_atleast(4, 15))
        {
            TestableEnumClass* enum_test = engine_test->get_property<TestableEnumClass>(L"m_testable_enum_class");
            Output::send(STR("{} (val: {} | expected: {}) <- {}\n"), (void*)enum_test, (int32_t)*enum_test, 1, L"m_testable_enum_class");
            assert_test(*enum_test == TestableEnumClass::ClassValueTwo, STR("EnumProperty<TestableEnumClass>"))
        }
        // UEnum Test -> END

        // MapProperty -> START
        auto testable_int_int_map = engine_test->get_property<TMap<int32_t, int32_t>>(L"m_testable_int_int_map");
        int32_t* testable_int_int_map_value_one = testable_int_int_map[3];
        int32_t* testable_int_int_map_value_two = testable_int_int_map[6];
        int32_t* testable_int_int_map_value_three = testable_int_int_map[9];
        Output::send(STR("testable_int_int_map_value_one: {}\n"), *testable_int_int_map_value_one);
        Output::send(STR("testable_int_int_map_value_two: {}\n"), *testable_int_int_map_value_two);
        Output::send(STR("testable_int_int_map_value_three: {}\n"), *testable_int_int_map_value_three);
        assert_test(testable_int_int_map_value_one && *testable_int_int_map_value_one == 1, STR("testable_int_int_map_value_one == 1"))
        assert_test(testable_int_int_map_value_two && *testable_int_int_map_value_two == 2, STR("testable_int_int_map_value_two == 2"))
        assert_test(testable_int_int_map_value_three && *testable_int_int_map_value_three == 3, STR("testable_int_int_map_value_three == 3"))
        // MapProperty -> END
    }

    auto UE4SSProgramTest::execute_function_call_tests() -> void
    {
        // Call to CharacterMovementComponent::SetMovementMode

        UCharacterMovementComponent* movement_component = static_cast<UCharacterMovementComponent*>(UObjectGlobals::find_first_of(L"CharacterMovementComponent"));

        if (!movement_component)
        {
            throw std::runtime_error{"Test failed: [Calling_CharacterMovementComponent_SetMovementMode] => movement_component is nullptr"};
        }

        uint8_t current_movement_mode = *movement_component->get_property<uint8_t>(L"MovementMode");

        // Set MovementMode to Flying if not Flying, otherwise set to Falling
        movement_component->SetMovementMode(current_movement_mode != 5 ? 5 : 3, 0);
    }
}

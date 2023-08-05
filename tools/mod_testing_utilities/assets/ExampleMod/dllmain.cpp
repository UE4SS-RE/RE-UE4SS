#include <Mod/CppUserModBase.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UObject.hpp>

using namespace RC;
using namespace RC::Unreal;

class ExampleMod : public RC::CppUserModBase
{
public:
    //Constructor
    ExampleMod() : CppUserModBase()
    {
        ModName = STR("ExampleMod");
        ModVersion = STR("1.0");
        ModDescription = STR("This is an example C++ mod");
        ModAuthors = STR("UE4SS Team");
        // Do not change this unless you want to target a UE4SS version
        // other than the one you're currently building with somehow.
        //ModIntendedSDKVersion = STR("2.6");
        
        Output::send<LogLevel::Verbose>(STR("ExampleMod says hello\n"));
    }
	
    //Deconstructor
    ~ExampleMod()
    {
    }
	
	auto on_unreal_init() -> void override
	{
		// You are allowed to use the 'Unreal' namespace in this function and anywhere else after this function has fired.
		auto Object = UObjectGlobals::StaticFindObject(nullptr, nullptr, STR("/Script/CoreUObject.Object"));
		Output::send<LogLevel::Verbose>(STR("Object Name: {}\n"), Object->GetFullName());
	}
	
    auto on_update() -> void override
    {
    }
};

#define EXAMPLE_MOD_API __declspec(dllexport)
extern "C"
{
    EXAMPLE_MOD_API RC::CppUserModBase* start_mod()
    {
        return new ExampleMod();
    }

    EXAMPLE_MOD_API void uninstall_mod(RC::CppUserModBase* mod)
    {
        delete mod;
    }
}
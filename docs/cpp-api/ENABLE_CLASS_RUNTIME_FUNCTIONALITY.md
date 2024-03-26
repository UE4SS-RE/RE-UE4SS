# ENABLE_CLASS_RUNTIME_FUNCTIONALITY

This macro is intended to be a generic way to enable extra functionality for custom UClass implementations.

## Current functionality

More functionality might be added in the future, and if that happens, it will be documented here.

### AActor

If your AActor class has a function named (exactly) `Tick` with a delta seconds param (float) and a return type of void, this function will automatically be called every time the actor ticks.

---

## Usage

The usage of this macro must take place in the `on_unreal_init` function of your mod class.  
The class provided to the macro must have been declared and implemented with the `DECLARE_EXTERNAL_OBJECT_CLASS`/`IMPLEMENT_EXTERNAL_OBJECT_CLASS` macros.  
Example:

```c++
// This is a class named 'AGame_Character'.
// It's created by the game devs.
// It extends UEs base 'ACharacter' class.
// This is only an example name.
// Your game will have a different name for its character class.
class AGame_Character : public RC::Unreal::AActor
{
private:
    DECLARE_EXTERNAL_OBJECT_CLASS(AGame_Character, Game)
    
public:
    void Tick(float DeltaSeconds)
    {
        Output::send(STR("AGame_Character Tick\n"));
    }
};
IMPLEMENT_EXTERNAL_OBJECT_CLASS(AGame_Character)

class MyAwesomeMod : public RC::CppUserModBase
{
public:
    MyAwesomeMod() : CppUserModBase()
    {
        ModName = STR("MyAwesomeMod");
        ModVersion = STR("1.0");
        ModDescription = STR("This is my awesome mod");
        ModAuthors = STR("UE4SS Team");
    }

    auto on_unreal_init() -> void override
    {
        ENABLE_CLASS_RUNTIME_FUNCTIONALITY(AGame_Character)
    }
};
```

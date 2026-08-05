# SDK Generator

This generator is meant to give easy access to all reflected member variables and functions for C++ mods.  
It also generates `enum` definitions for all reflected enums.  
The generated structs and classes are memory accurate: leading, inter-member and trailing padding are all emitted, static arrays and bitfields are reproduced, and struct alignment is computed rather than assumed.

Where a derived type places its members inside a base type's trailing padding (which MSVC allows for non-POD types), the base is emitted packed with an explicit `alignas` so the derived members still land at the right offsets.

A property whose C++ type the generator cannot express — a `TMap`/`TSet` member, a Verse property, or a property class the running engine defines and this generator doesn't know — is emitted as an opaque stand-in from `UE4SS_SDK/PlaceholderTypes.hpp` with the size and alignment reported by the engine. The member keeps its name and offset, everything after it stays correctly placed, and `GetTyped<T>()` reinterprets the storage as your own definition if you write one. Previously a single such property aborted the entire dump and produced no files at all.

To verify a generated SDK against a specific game build, include `UE4SS_SDK/LayoutAsserts.hpp`. It contains `static_assert`s for the size, alignment and every member offset of every generated type. It is not included by default, so a layout problem shows up as a failed assert on the exact type rather than breaking the whole SDK build. `UE4SS_SDK/RuntimeSDKTest.hpp` performs the equivalent check at runtime against the live game.

The UE4SS backend is compiled into UE4SS, so the generator works without any configuration files present. Dropping a backend description into `<working_dir>/UE4SS_SDK_Backends` adds a new backend, or overrides the built-in one if it uses the same file name (`UE4SS.json`). The shipped copy in `assets/UE4SS_SDK_Backends/UE4SS.json` is the same file the built-in is generated from, so it doubles as a starting point for customisation.

## How it works

The generator generates classes, struct and enums.  
It was designed to be used in a C++ mod with UE4SS as the UE backend without much effort.  
If you don't want to use UE4SS as your backend, it's up to you to supply all needed UE types like `UObject`, as well as templated types like `TArray`, `TObjectPtr`, and `TWeakObjectPtr`.  
You must also supply UEs `FindObject<T>` function template as that's how functions are found at runtime.  
This call must be supported in your backend: `FindObject<UFunction>(nullptr, L"/Script/Engine.CharacterMovementComponent:SetMovementMode")`

## Usage

The following guide is meant for use with a UE4SS C++ mod, but it can be modified to work with any UE backend.  
Have a look at `UE4SS-settings.ini` in the `[SDKGenerator]` section for more information on how to make this work with a different backend.

1. Make sure you've got `GuiConsoleEnabled` and `GuiConsoleVisible` set to 1, and then start the game.
2. Go to the `Dumpers` tab  in the GUI and click the `Generate SDK` button.  
   There should now be a `UE4SS_SDK` directory in `<Game>/Binaries/Win64`.
3. Copy this entire directory into your C++ mod directory.
4. In the `CMakeLists.txt` for your C++ mod, add `add_subdirectory(UE4SS_SDK)` and make sure to add `UE4SS_SDK` to your `target_link_libraries`.

## Usage in C++

To use the SDK in your code, you must include the parts of it that you're interested in.

Here's an example, it's accurate regardless of what backend you've chosen to use.  
The only difference between using UE4SS as your backend or any other backend is where the base UE types are coming from.  
This won't work if you use UE4SS as your backend as the `AActor` type is already provided by UE4SS, but the example will work for any type that UE4SS doesn't provide:

```c++
#include <UE4SS_SDK/Script/Engine/Actor.hpp>

// This example is abbreviated, and doesn't include 'int main()'
// or any other unrelated but required code.

void DoStuff()
{
    // For this example, assume 'Actor' is a valid AActor.
    AActor* Actor{};
    
    // The 'GetChildren' function returns 'TArray<AActor*>', so you must have provided 'TArray<T>'.
    auto& Array = AsActor->Children;
    for (const auto& Element : Array)
    {
        printf("Element: %S", Element->GetFullName().c_str());
    }
}
```

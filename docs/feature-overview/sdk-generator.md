# SDK Generator

This generator is meant to give easy access to all reflected member variables and functions for C++ mods.  
It also generates `enum` definitions for all reflected enums.  
The generated structs and classes as mostly memory accurate, but we don't handle padding at the end of structs/classes which means that some variables might be misaligned by a few bytes. 

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

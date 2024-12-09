# Fixing missing AOBs

If UE4SS won't properly start because of missing AOBs, you can provide your own AOB and callback using Lua.

For this guide you'll need to know what a `root directory` and `working directory` is.
A `root directory` is always the directory that contains `ue4ss.dll`.
A `working directory` is either the directory that contains `ue4ss.dll` OR a game specific directory, for example `<root directory>/SatisfactoryEarlyAccess`.

## How to find AOBs

> [!CAUTION]
> Finding AOBs for a game is no simple task and requires research into basic reverse engineering principles. It's not something for which we can just make an all-encompassing guide.

Since the process is quite complicated, here will just cover the general steps you need to take.

1. Make a blank shipped game in your game's UE version, with PDBs
2. Read game's memory using x64dbg
3. Look for the signature you need - those can be found [below](#what-onmatchfound-must-return-for-each-aob)
4. Grab a copy of the bytes from that function, sometimes the header is enough. If it is not, it may be better to grab a call to the function and if it's not a virtual function, you can grab the RIP address there
5. Open your game's memory in x64dbg and search it for the same block of bytes
6. If you find it, you can use the [swiss army knife](https://github.com/Nukem9/SwissArmyKnife) tool to extract the AOB for it which you can use in a simple script such as example [here](#example-script-simple-direct-scan)

### Context and definitions

Some context and definitions:

In this context, a `Signature` refers to a unique sequence or pattern of bytes used to identify a function or piece of code within a binary, such as specific instructions or constants that are unlikely to appear elsewhere. It serves as a recognizable "fingerprint" to locate a particular routine during reverse engineering or patching.

In contrast, a `Block of Bytes` is simply a contiguous sequence of raw data or instructions without any specific identification purpose. A block of bytes may or may not represent anything meaningful or unique, whereas a signature is carefully chosen to reliably distinguish a particular function or code segment.

`RIP (Instruction Pointer Register)` is a register in x86-64 architecture that holds the address of the next instruction to be executed. It plays a key role in managing program flow, enabling the CPU to keep track of where it is in the program code.

Now for each step in more detail (thanks for `TimeMaster` for these steps).

### Making a blank shipped game

1. Get your game UE version. UE4SS detects it. But it can also be checked by using right-click on the `.exe` in `Binaries`, opening properties and checking on the details tab
2. In the Epic Games launcher at the left side, go to Unreal Engine -> Library tab at the top and install engine version for the engine version for your game
3. Once installed launch Unreal Engine. Games tab -> Select Blank -> Uncheck Starter Content (Optional to set a Project Name / change location) -> Create
4. Press Platforms button on the top bar -> Packaging Settings -> Check `Include Debug Files in Shipping Builds`
5. Press Platforms button on the top bar -> Windows -> Select `Shipping` (or the one that applies to your game build) -> Package Project and select a folder
6. Check that the newly packaged blank project contains a `.exe` along with a `.pdb` in `Binaries` in the selected folder

### Reading the game's memory using x64dbg

1. Install [x64dbg](https://x64dbg.com/)
2. Run the `.exe` at the root folder of the newly packaged blank project (running the `.exe` in `Binaries` might throw an error, running from root works too either way)
3. Open x64dbg -> File -> Attach -> Select the newly packaged blank project `.exe` (the one with the path at `Binaries`)

### Look for the signature you need

1. (Optional but recommended) Connect Epic Games with Github. Login in the Epic Games Website -> Manage Account -> Apps and Accounts -> Github -> Once done, check email and accept invitation to the UE project
2. (Optional but recommended) Check the source code for the function that is intended to be found in memory. For example, to find the `FMemory::Free` function in a UE5.3.2 game, you would find [this](https://github.com/EpicGames/UnrealEngine/blob/5.3.2-release/Engine/Source/Runtime/Core/Public/HAL/FMemory.inl#L142)
3. In x64dbg go to Symbols tab -> In the left window select the `.exe` -> Under the right window search for the function (in this case `FMemory::Free`) -> Double click the found Function in the right window
4. You should be now back at the CPU tab with the address in memory of the start of the selected function

### Grab a copy of bytes from the function

1. (Optional but recommended) Install [Baymax ToOls](https://github.com/sicaril/BaymaxTools) plugin for x64dbg
2. Select some (This is where it is not the same for every game and required magic/"knowledge" starts) address lines -> Right Click -> Copy -> Selection or Selection (Bytes only)
3. If Baymax ToOls installed, while selecting all the addresses lines composing the function -> Right Click -> Baymax ToOls -> Copy Signature.
4. Might want to copy both selection types and save them in a file for comparison and reference

### Open your game's memory in x64dbg

1. Open the game you want to mod
2. Attach x64dbg as seen before with the blank project
3. Search for the saved block of bytes found in the last step
4. (If nothing found) Search for the pattern from Baymax ToOls
5. (If nothing found) Try searching parts of the block of bytes (or signature from Baymax ToOls) and compare the addresses block with the one from the blank project
6. If nothing found, it might be worth to ask for help on the UE4SS discord or Github issues. Make sure you post all your steps and as much detail as you can provide, otherwise no one will be inclined to help you!
7. If found a good match, create the lua script to retrieve the address of the function/variable required. Put it in `UE4SS_Signatures` folder in the `Binaries` of your game folder where UE4SS is installed
8. Run the game and UE4SS hopefully works now

## How to setup your own AOB and callback

1. Create the directory `UE4SS_Signatures` if it doesn't already exist in your `working directory`.
2. Identify which AOBs are broken and needs fixing.
3. Make the following files inside `UE4SS_Signatures`, depending on which AOBs are broken:
    - GUObjectArray.lua
    - FName_ToString.lua
    - FName_Constructor.lua
    - FText_Constructor.lua
    - StaticConstructObject.lua
    - GMalloc.lua
4. Inside the `.lua` file you need a global `Register` function with no params
    - Keep in mind that the names of functions in Lua files in the `UE4SS_Signatures` directory are case-senstive.
5. The `Register` function must return the AOB that you want UE4SS to scan for.
    - The format is a list of nibbles, and every two forms a byte.  
    - I like putting a space between each byte just for clarity but this is not a requirement. 
    - An example of an AOB: `8B 51 04 85`.
    - Another example of an AOB: `8B510485`.
    - The AOB scanner supports wildcards for either nibble or the entire byte.
6. Next you need to create a global `OnMatchFound` function.
    - This function has one param, `MatchAddress`, and this is the address of the match.
    - It's in this function that you'll place all your logic for calculating the final address.
    - The most simple way to do this is to make sure that your AOB leads directly to the start of the final address. That way you can simply return `MatchAddress`.
    - In the event that you're doing something more advanced (e.g. indirect aob scan), UE4SS makes available two global functions, `DerefToInt32` which takes an address and returns, as a 32-bit integer, whatever data is located there OR `nil` if the address could not be dereferenced, and `print` for debugging purposes.

## What 'OnMatchFound' must return for each AOB
- GUObjectArray
   - Must return the exact address of the global variable named 'GUObjectArray'.
- FName_ToString
   - Must return the exact address of the start of the function 'FName::ToString'.  
     Function signature: `public: void cdecl FName::ToString(class FString & ptr64)const __ptr64`
- FName_Constructor
   - Must return the exact address of the start of the function 'FName::FName'.  
     This callback is likely to be called many times and we do a check behind the scenes to confirm if we found the right constructor.  
     It doesn't matter if your AOB finds both 'char*' versions and 'wchar_t*' versions.  
     Function signature: `public: cdecl FName::FName(wchar_t const * ptr64,enum EFindName) __ptr64`
- FText_Constructor
  - Must return the exact address of the start of the function 'FText::FText'.  
    Function signature: `public: cdecl FText::FText(class FString & ptr64)const __ptr64`
- StaticConstructObject
   - Must return the exact address of the start of the global function 'StaticConstructObject_Internal'.  
     In UE4SS, we scan for a call in the middle of 'UUserWidget::InitializeInputComponent' and then resolve the call location.  
     Function signature: `class UObject * __ptr64 __cdecl StaticConstructObject_Internal(struct FStaticConstructObjectParameters const & __ptr64)`
- GMalloc
     - Must return the exact address of the global variable named 'GMalloc'.  
     In UE4SS, we scan for `FMemory::Free` and then resolve the MOV instruction closest to the first CALL instruction.

## Example script (Simple, direct scan)

```lua
function Register()
    return "48 8B C4 57 48 83 EC 70 80 3D ?? ?? ?? ?? ?? 48 89"
end

function OnMatchFound(MatchAddress)
    return MatchAddress
end
```

## Example script (Advanced, indirect scan)

```lua
function Register()
    return "41 B8 01 00 00 00 48 8D 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E9"
end

function OnMatchFound(MatchAddress)
    local InstrSize = 0x05

    local JmpInstr = MatchAddress + 0x14
    local Offset = DerefToInt32(JmpInstr + 0x1)
    local Destination = JmpInstr + Offset + InstrSize

    return Destination
end
```

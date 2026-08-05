# RE-UE4SS Upgrade Guide

This document provides detailed guidance for upgrading between versions of RE-UE4SS. Each section covers a specific version upgrade and describes breaking changes, deprecations, and the steps required to migrate your code successfully.

## Version 3.x to 4.x

> **Released:** [TBD]  
> **PR:** [Link to PR when available]

### Breaking Changes

#### FName Constructor Default Parameter Change

**What Changed:**  
The default `EFindName` parameter for FName constructors has changed from `FNAME_Find` to `FNAME_Add`.

**Before (v3.x):**
```cpp
// Default was FNAME_Find - only found existing names
explicit FName(const CharType* StrName, EFindName FindType = FNAME_Find, void* FunctionAddressOverride = nullptr);
```

**After (v4.x):**
```cpp
// Default is now FNAME_Add - creates name if it doesn't exist
explicit FName(const CharType* StrName, EFindName FindType = FNAME_Add, void* FunctionAddressOverride = nullptr);
```

**Migration Steps:**

1. If you rely on the old behavior of FName constructors returning NAME_None when a name doesn't exist:
   ```cpp
   // Old (implicit FNAME_Find)
   FName TestName(STR("NonExistentName")); // Would result in NAME_None if name doesn't exist
   
   // New (explicit FNAME_Find to maintain old behavior)
   FName TestName(STR("NonExistentName"), FNAME_Find); // Explicitly specify FNAME_Find
   ```

2. If you're checking for name validity after construction:
   ```cpp
   // Old code that might break
   FName TestName(STR("PotentiallyNewName"));
   if (!TestName) { // This check no longer works as expected
       // Handle invalid name
   }
   
   // New approach
   FName TestName(STR("PotentiallyNewName"), FNAME_Find); // Use FNAME_Find if you need to check existence
   if (!TestName) {
       // Handle non-existent name
   }
   ```

3. For code that already explicitly specified the FindType parameter, no changes are needed.

**Important Notes:**
- This change affects all three FName constructor overloads that accept string parameters
- The new default behavior (`FNAME_Add`) will create new FName entries in the name table if they don't exist
- This can have performance implications if you're frequently creating FNames with strings that may not exist
- When working with Unreal Engine internals, be cautious as creating new names unintentionally could lead to unexpected behavior



#### TObjectPtr Implementation Change
**What Changed:**  
The `TObjectPtr<>` class has been enhanced to function as a proper smart pointer instead of a simple wrapper.

**Before (v3.x):**
```cpp
// Simple class that makes everything compile.
template<typename UnderlyingType>
class TObjectPtr
{
public:
    UnderlyingType* UnderlyingObjectPointer;
};
```

**After (v4.x):**
```cpp
template<typename T>
class TObjectPtr
{
public:
    using ElementType = T;
    // Constructors
    TObjectPtr() : Ptr(nullptr) {}
    TObjectPtr(TYPE_OF_NULLPTR) : Ptr(nullptr) {}
    TObjectPtr(const TObjectPtr& Other) : Ptr(Other.Ptr) {}
    explicit TObjectPtr(ElementType* InPtr) : Ptr(InPtr) {}

    // Assignment operators
    TObjectPtr& operator=(const TObjectPtr& Other) { Ptr = Other.Ptr; return *this; }
    TObjectPtr& operator=(TYPE_OF_NULLPTR) { Ptr = nullptr; return *this; }
    TObjectPtr& operator=(ElementType* InPtr) { Ptr = InPtr; return *this; }
        
    // Conversion operators
    FORCEINLINE operator T* () const { return Get(); }
    template <typename U>
    UE_OBJPTR_DEPRECATED(5.0, "Explicit cast to other raw pointer types is deprecated.  Please use the Cast API or get the raw pointer with ToRawPtr and cast that instead.")
    explicit FORCEINLINE operator U* () const { return (U*)Get(); }
    explicit FORCEINLINE operator UPTRINT() const { return (UPTRINT)Get(); }
    FORCEINLINE T* operator->() const { return Get(); }
    FORCEINLINE T& operator*() const { return *Get(); }

    // Comparison operators
    bool operator==(const TObjectPtr& Other) const { return Ptr == Other.Ptr; }
    bool operator!=(const TObjectPtr& Other) const { return Ptr != Other.Ptr; }
    bool operator==(const ElementType* InPtr) const { return Ptr == InPtr; }
    bool operator!=(const ElementType* InPtr) const { return Ptr != InPtr; }
    bool operator==(TYPE_OF_NULLPTR) const { return Ptr == nullptr; }
    bool operator!=(TYPE_OF_NULLPTR) const { return Ptr != nullptr; }

    // Additional API compatibility with UE's TObjectPtr
    bool operator!() const { return Ptr == nullptr; }
    explicit operator bool() const { return Ptr != nullptr; }
    ElementType* Get() const { return Ptr; }

private:
    ElementType* Ptr;
};
```

**Migration Steps:**
1. For direct pointer access (previously accessed via `UnderlyingObjectPointer`):
   ```cpp
   // Old
   TObjectPtr<UClass> ClassPtr;
   UClass* RawPtr = ClassPtr.UnderlyingObjectPointer;
   
   // New
   TObjectPtr<UClass> ClassPtr;
   UClass* RawPtr = ClassPtr; // implicit conversion
   // or
   UClass* RawPtr = ClassPtr.Get(); // explicit access
   ```

2. For address-of operations on TObjectPtr contents:
   ```cpp
   // Old
   &ObjectPtr.UnderlyingObjectPointer
   
   // New
   &ObjectPtr.Get()
   // or simply
   &ObjectPtr
   ```

3. For pointer-to-integer conversions (e.g., formatting addresses):
    ```cpp
    // Old
    uintptr_t addr = reinterpret_cast<uintptr_t>(ObjectPtr.UnderlyingObjectPointer);

    // New
    uintptr_t addr = reinterpret_cast<uintptr_t>(ObjectPtr.Get());
    // or
    uintptr_t addr = reinterpret_cast<uintptr_t>(ToRawPtr(ObjectPtr)); // using helper to extract a raw pointer from TObjectPtr
    ```

#### FString API Changes

##### `GetCharArray()` Behavior Change

**What Changed:**  
The `GetCharArray()` method now returns the actual array instead of a pointer to characters.

**Before (v3.x):**
```cpp
// Returned a character pointer
auto GetCharArray() const -> const CharType*;
```

**After (v4.x):**
```cpp
// Returns the array itself
FORCEINLINE DataType& GetCharArray() { return Data; }
FORCEINLINE const DataType& GetCharArray() const { return Data; }

// Use operator* for character pointer access
FORCEINLINE const TCHAR* operator*() const { return Data.Num() ? Data.GetData() : TEXT(""); }
```

**Migration Steps:**

1. For character pointer access (previously `GetCharArray()`):
   ```cpp
   // Old
   const TCHAR* ptr = myString.GetCharArray();
   
   // New
   const TCHAR* ptr = *myString;
   ```

2. For array access (previously `GetCharTArray()`):
   ```cpp
   // Old
   TArray<TCHAR>& arr = myString.GetCharTArray();
   
   // New
   TArray<TCHAR>& arr = myString.GetCharArray();
   ```

#### FSoftObjectPath Member Variables Replaced By Accessors

**What Changed:**  
`FSoftObjectPath` no longer exposes the `AssetPathName` and `SubPathString` member variables. The struct now stores opaque bytes and resolves the engine's actual layout at runtime, because UE 5.1 changed the layout (`FName AssetPathName` became `FTopLevelAssetPath AssetPath`, moving `SubPathString` from offset 0x8 to 0x10). The old compile-time members read the wrong memory on every 5.1+ game.

**Before (v3.x):**
```cpp
FSoftObjectPath Path = ...;
FName AssetName = Path.AssetPathName;
FString& SubPath = Path.SubPathString;
Path.SubPathString = NewSubPath;
```

**After (v4.x):**
```cpp
FSoftObjectPath Path = ...;
FName AssetName = Path.GetAssetPathName();
FString& SubPath = Path.GetSubPathString();
Path.GetSubPathString() = NewSubPath;

// New: package/asset name pair, works on every engine version
FTopLevelAssetPath AssetPath = Path.GetAssetPath();
```

**Migration Steps:**

1. Replace `Path.AssetPathName` with `Path.GetAssetPathName()`.
2. Replace `Path.SubPathString` with `Path.GetSubPathString()`. A non-const overload exists, so reads, writes, and passing by reference all work.
3. If you need the package and asset names separately, prefer `GetAssetPath()`, which returns an `FTopLevelAssetPath` in every engine version (synthesized by splitting the path on pre-5.1).

**Important Notes:**
- On 5.1+ engines there is no single-FName representation in memory, so `GetAssetPathName()` creates the combined `/Package/Path.Asset` FName on demand (with `FNAME_Add`).
- Writing the asset path goes through `SetPath()` or `operator=(const FString&)`, unchanged from before.
- `TArray<FSoftObjectPath>` now strides by the runtime size (`FSoftObjectPath::StaticSize()`), fixing element access on 5.1+ and case-preserving builds.
- The Lua API (`GetAssetPathName()`, `GetSubPathString()`) is unchanged; Lua mods are unaffected.

#### FText Member Variables Replaced By Accessors

**What Changed:**  
`FText` no longer exposes the `Data`, `SharedRefCollector`, `Flags` and `Unk` member variables. UE 5.4 changed the internal representation from `TSharedRef<ITextData>` (FText is 0x18 bytes) to `TRefCountPtr<ITextData>` (0x10 bytes), so a fixed member layout cannot be correct everywhere. FText now stores opaque bytes sized by `FText::StaticSize()`, and copies, assignments and destruction go through the engine's own `FTextProperty` value operations, which means FText copies are properly reference counted in every engine version.

**Before (v3.x):**
```cpp
FText Text = ...;
ITextData* Data = Text.Data;
uint32 Flags = Text.Flags;
```

**After (v4.x):**
```cpp
FText Text = ...;
ITextData* Data = Text.GetTextData();
uint32 Flags = Text.GetFlags();
```

**Migration Steps:**

1. Replace `Text.Data` with `Text.GetTextData()`.
2. Replace `Text.Flags` with `Text.GetFlags()`.
3. Remove any use of `SharedRefCollector` or `Unk`; they were artifacts of the guessed layout. The reference controller is engine-managed now.
4. Code that constructs, copies, assigns, or converts FText (`ToString()`, `ToFString()`, `SetString()`, `FText(FString)`) requires no changes.

**Important Notes:**
- Copies made through C++ are now owning references; they are released automatically when the FText is destroyed. Byte-wise duplication of FText values (memcpy into a buffer that outlives the source) should be replaced with assignment, or with `FText::StealFromBuffer` / `CopyBorrowedTo` when interacting with engine param buffers.
- Passing FText by value to engine functions works unchanged; the temporary carries its own reference.
- The Lua API is unchanged; Lua mods are unaffected.

### Deprecations

- `GetCharTArray()` is now deprecated but still available for compatibility.
  It forwards to `GetCharArray()` and will be removed in a future version.

#### Property Header Consolidation

**What Changed:**
Individual property header files (`Unreal/Property/*.hpp` and `Unreal/FProperty.hpp`) are deprecated. Nearly all property types are now consolidated in `Unreal/CoreUObject/UObject/UnrealType.hpp`.

**Before (v3.x):**
```cpp
#include <Unreal/FProperty.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
// etc.
```

**After (v4.x):**
```cpp
// Single include for all property types
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
```

**Migration Steps:**

1. Replace any of the below property-related includes with `Unreal/CoreUObject/UObject/UnrealType.hpp`:
   ```cpp
   // Remove these deprecated includes:
   // #include <Unreal/FProperty.hpp>
   // #include <Unreal/Property/FArrayProperty.hpp>
   // #include <Unreal/Property/FBoolProperty.hpp>
   // #include <Unreal/Property/FClassProperty.hpp>
   // #include <Unreal/Property/FDelegateProperty.hpp>
   // #include <Unreal/Property/FInterfaceProperty.hpp>
   // #include <Unreal/Property/FLazyObjectProperty.hpp>
   // #include <Unreal/Property/FMapProperty.hpp>
   // #include <Unreal/Property/FMulticastDelegateProperty.hpp>
   // #include <Unreal/Property/FMulticastInlineDelegateProperty.hpp>
   // #include <Unreal/Property/FMulticastSparseDelegateProperty.hpp>
   // #include <Unreal/Property/FNameProperty.hpp>
   // #include <Unreal/Property/FNumericProperty.hpp>
   // #include <Unreal/Property/FObjectProperty.hpp>
   // #include <Unreal/Property/FSetProperty.hpp>
   // #include <Unreal/Property/FSoftClassProperty.hpp>
   // #include <Unreal/Property/FSoftObjectProperty.hpp>
   // #include <Unreal/Property/FStructProperty.hpp>
   // #include <Unreal/Property/FWeakObjectProperty.hpp>
   // #include <Unreal/Property/NumericPropertyTypes.hpp>

   // Replace with:
   #include <Unreal/CoreUObject/UObject/UnrealType.hpp>
   ```

2. No code changes are required - all property types are still available with the same names and APIs.

**Why This Change:**
- Simplifies include dependencies
- Reduces compilation times
- Matches Unreal Engine's own header organization where nearly all property types are defined in `UnrealType.h`
- Eliminates include ordering issues

#### Property Iteration API Improvements

**What Changed:**
While the existing `ForEachProperty()` and `ForEachPropertyInChain()` methods are still available and fully supported, we now recommend using Epic's `TFieldIterator<>` and `TFieldRange<>` patterns directly for consistency with Unreal Engine code and clearer iteration control.

**Existing API (still supported):**
```cpp
// Iterator-based iteration (still works)
for (FProperty* Property : MyStruct->ForEachProperty())
{
    // Process property
}

for (FProperty* Property : MyStruct->ForEachPropertyInChain())
{
    // Process property including super classes
}
```

**Recommended API (new in v4.x):**
```cpp
// TFieldIterator - Epic's pattern for child->parent iteration
#include <Unreal/UnrealType.hpp>

// Iterate properties in this class only (excludes super classes)
for (FProperty* Property : TFieldRange<FProperty>(MyStruct, EFieldIterationFlags::None))
{
    // Process property
}

// Iterate all properties including super classes (child->parent order)
for (FProperty* Property : TFieldRange<FProperty>(MyStruct, EFieldIterationFlags::IncludeSuper))
{
    // Process property including inheritance chain
}

// Include deprecated properties
for (FProperty* Property : TFieldRange<FProperty>(MyStruct, EFieldIterationFlags::IncludeDeprecated))
{
    // Process all properties including deprecated ones
}

// Combined flags
for (FProperty* Property : TFieldRange<FProperty>(MyStruct, EFieldIterationFlags::IncludeSuper | EFieldIterationFlags::IncludeDeprecated))
{
    // Process all properties in chain including deprecated
}

// Iterate in parent->child order (reverse)
for (FProperty* Property : TReverseFieldRange<FProperty>(MyStruct, EFieldIterationFlags::None))
{
    // Process properties from parent to child
}

// You can also iterate other field types
for (UFunction* Function : TFieldRange<UFunction>(MyClass, EFieldIterationFlags::IncludeSuper))
{
    // Process functions
}
```

**EFieldIterationFlags Options:**
```cpp
enum EFieldIterationFlags : uint8
{
    None                 = 0,      // Only iterate fields in this struct/class
    IncludeSuper        = 1 << 0,  // Include inherited fields from super classes
    IncludeDeprecated   = 1 << 1,  // Include deprecated properties
    IncludeInterfaces   = 1 << 2,  // Include interface properties (classes only)

    // Convenience aliases
    Default = IncludeSuper,        // Default Epic behavior
};
```

**Migration Examples:**

1. Simple property iteration:
   ```cpp
   // Old (still works)
   for (FProperty* Prop : MyStruct->ForEachProperty())
   {
       Output::send(STR("{}\n"), Prop->GetName());
   }

   // New (recommended)
   for (FProperty* Prop : TFieldRange<FProperty>(MyStruct, EFieldIterationFlags::None))
   {
       Output::send(STR("{}\n"), Prop->GetName());
   }
   ```

2. Iteration with inheritance:
   ```cpp
   // Old (still works)
   for (FProperty* Prop : MyStruct->ForEachPropertyInChain())
   {
       Output::send(STR("{}\n"), Prop->GetName());
   }

   // New (recommended)
   for (FProperty* Prop : TFieldRange<FProperty>(MyStruct, EFieldIterationFlags::IncludeSuper))
   {
       Output::send(STR("{}\n"), Prop->GetName());
   }
   ```

3. Parent-to-child iteration order:
   ```cpp
   // Old (still works)
   for (FProperty* Prop : OrderedForEachPropertyInChain(MyStruct))
   {
       Output::send(STR("{}\n"), Prop->GetName());
   }

   // New (recommended)
   for (FProperty* Prop : TReverseFieldRange<FProperty>(MyStruct, EFieldIterationFlags::None))
   {
       Output::send(STR("{}\n"), Prop->GetName());
   }
   ```

4. Using the iterator directly:
   ```cpp
   // More control with explicit iterator
   for (TFieldIterator<FProperty> It(MyStruct, EFieldIterationFlags::IncludeSuper); It; ++It)
   {
       FProperty* Prop = *It;
       Output::send(STR("{}\n"), Prop->GetName());
   }
   ```

**Why Consider The New API:**
- **Consistency:** Matches Epic's UE4/UE5 codebase patterns exactly
- **Flexibility:** Fine-grained control over what to include (super classes, deprecated properties, interfaces)
- **Type-safe:** Can iterate any field type (`FProperty`, `UFunction`, `UEnum`, etc.)
- **Familiarity:** If you know Epic's API, you already know this pattern
- **Explicitness:** Makes iteration behavior clear at the call site

**Important Notes:**
- The old `ForEachProperty()` methods are **deprecated** and may be removed in the future
- Use `TFieldIterator`/`TFieldRange` for better performance and when matching Epic's patterns

### Deprecations

#### ExecuteAsync and LoopAsync

**What Changed:**
`ExecuteAsync` and `LoopAsync` are now deprecated in favor of the new Delayed Action System. While these functions still work, they lack the control and safety features of the new system.

**Why Migrate:**
- **Cancellation:** Old functions cannot be cancelled once started
- **Pause/Resume:** No way to pause async loops
- **Query State:** Cannot check if an action is pending or get remaining time
- **Mod Isolation:** Old functions don't have ownership tracking
- **Thread Safety:** New system has better thread safety, particularly when interacting with game objects

**Migration Guide:**

| Old Function | New Function |
|--------------|--------------|
| `ExecuteAsync(delayMs, callback)` | `ExecuteInGameThreadWithDelay(delayMs, callback)` |
| `LoopAsync(delayMs, callback)` | `LoopInGameThreadWithDelay(delayMs, callback)` |

```lua
-- OLD (deprecated)
ExecuteAsync(1000, function()
    print("After 1 second\n")
end)

LoopAsync(1000, function()
    print("Every second\n")
    return false  -- return true to stop
end)

-- NEW (recommended)
ExecuteInGameThreadWithDelay(1000, function()
    print("After 1 second\n")
end)

local loopHandle
loopHandle = LoopInGameThreadWithDelay(1000, function()
    print("Every second\n")
    if shouldStop then
        CancelDelayedAction(loopHandle)
    end
end)
```

**Benefits of Migration:**
- Cancel loops anytime with `CancelDelayedAction(handle)`
- Pause/resume with `PauseDelayedAction(handle)` and `UnpauseDelayedAction(handle)`
- Query state with `IsDelayedActionActive(handle)`, `GetDelayedActionTimeRemaining(handle)`, etc.
- Use `ClearAllDelayedActions()` to clean up all your mod's timers at once

### New Features

#### Delayed Action System

**What's New:**
A comprehensive timer/delayed action system has been added to the Lua API. This provides UE-style delayed execution with full control over timing, pausing, cancellation, and looping. **This is the recommended replacement for `ExecuteAsync` and `LoopAsync`.**

**New Functions:**
- `ExecuteInGameThreadWithDelay(delayMs, callback)` - Execute callback after delay, returns handle
- `ExecuteInGameThreadWithDelay(handle, delayMs, callback)` - Execute only if handle not active (UE Delay-style)
- `RetriggerableExecuteInGameThreadWithDelay(handle, delayMs, callback)` - Resets timer if called again
- `LoopInGameThreadWithDelay(delayMs, callback)` - Repeating timer
- `ExecuteInGameThreadAfterFrames(frames, callback)` - Frame-based delay
- `LoopInGameThreadAfterFrames(frames, callback)` - Frame-based repeating timer
- `MakeActionHandle()` - Generate unique handle for use with delay functions

**Timer Control Functions:**
- `CancelDelayedAction(handle)` - Cancel a pending action
- `PauseDelayedAction(handle)` - Pause a timer
- `UnpauseDelayedAction(handle)` - Resume a paused timer
- `ResetDelayedActionTimer(handle)` - Restart with original delay
- `SetDelayedActionTimer(handle, newDelayMs)` - Change delay and restart
- `ClearAllDelayedActions()` - Cancel all actions for current mod

**Query Functions:**
- `IsValidDelayedActionHandle(handle)` - Check if handle exists
- `IsDelayedActionActive(handle)` - Check if timer is running
- `IsDelayedActionPaused(handle)` - Check if timer is paused
- `GetDelayedActionRate(handle)` - Get configured delay
- `GetDelayedActionTimeRemaining(handle)` - Get remaining time
- `GetDelayedActionTimeElapsed(handle)` - Get elapsed time

**New Global Variables:**
- `EngineTickAvailable` - Boolean indicating if EngineTick hook is available
- `ProcessEventAvailable` - Boolean indicating if ProcessEvent hook is available

**New Enums:**
- `EGameThreadMethod.ProcessEvent` - Use ProcessEvent hook
- `EGameThreadMethod.EngineTick` - Use EngineTick hook (once per frame)

**ExecuteInGameThread Enhancement:**
`ExecuteInGameThread` now accepts an optional second parameter to specify the execution method:
```lua
-- Default behavior (uses config setting)
ExecuteInGameThread(function() print("Hello\n") end)

-- Explicit method selection
ExecuteInGameThread(function() print("Hello\n") end, EGameThreadMethod.EngineTick)
ExecuteInGameThread(function() print("Hello\n") end, EGameThreadMethod.ProcessEvent)
```

**Example Usage:**
```lua
-- Simple delay
local handle = ExecuteInGameThreadWithDelay(1000, function()
    print("Fired after 1 second\n")
end)

-- Retriggerable timer (resets each call)
local debounceHandle = MakeActionHandle()
RegisterKeyBind(Key.F, function()
    RetriggerableExecuteInGameThreadWithDelay(debounceHandle, 500, function()
        print("Debounced action\n")
    end)
end)

-- Self-cancelling loop
local loopHandle
loopHandle = LoopInGameThreadWithDelay(1000, function()
    print("Tick!\n")
    if someCondition then
        CancelDelayedAction(loopHandle)
    end
end)
```

**Important Notes:**
- Handle-based ownership prevents mods from interfering with each other's timers
- `ClearAllDelayedActions()` only clears actions belonging to the calling mod
- Frame-based delays require EngineTick hook (check `EngineTickAvailable`)
- When capturing loop handles in closures, declare the variable before assignment:
  ```lua
  local loopHandle  -- Declare first
  loopHandle = LoopInGameThreadWithDelay(1000, function()
      CancelDelayedAction(loopHandle)  -- Now correctly captured
  end)
  ```

## Reporting Migration Issues

If you encounter problems while upgrading, please:

1. Check the [open issues](https://github.com/UE4SS-RE/RE-UE4SS/issues) for similar reports
2. Create a new upgrade problem issue if your issue hasn't been reported

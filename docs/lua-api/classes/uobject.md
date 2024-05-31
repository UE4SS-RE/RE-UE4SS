# UObject

The `UObject` class is the base class that most other Unreal Engine game objects inherit from.

## Inheritance
[RemoteObject](./remoteobject.md)

## Metamethods

### __index

- **Usage:** `UObject["ObjectMemberName"]` or `UObject.ObjectMemberName`

- Returns either a member variable (reflected property or custom property) or a UFunction.

- This method can return any type, and you can use the UObject-specific `type()` function on the returned value to figure out the type if the type is non-trivial. 

- If the type is trivial, use the regular `type()` Lua function.

- **Return Value:**

| # | Type                 | Information |
|---|----------------------|-------------|
| 1 | UObject or UFunction | If the type is `UObject`, then the actual type may be any class that inherits from `UObject`. |

- **Example:**
    ```lua
    local Character = FindFirstOf("Character")

    -- Retrieve a non-trivial type
    local MovementComponent = Character.CharacterMovement

    -- Retrieve a trivial type
    local JumpMaxCount = Character.JumpMaxCount

    -- Call a UFunction member on the object
    -- Remember to use a colon (:) for calls
    local CanCharacterJump = Character:CanJump()

    ```

### __newindex

- **Usage:** `UObject["ObjectMemberName"] = NewValue` or `UObject.ObjectMemberName = NewValue`

- Sets the value of a member variable to `NewValue`.

- **Example:**
    Sets the value of `MaxParticleResize` in the first instance of class `UEngine` in memory.
    ```lua
    local Engine = FindFirstOf("Engine")
    Engine.MaxParticleResize = 4
    ```

## Methods

### GetFullName()

- **Returns:** the full name & path info for a `UObject` & its derivatives

- **Return Value:** 

| # | Type                 | Information |
|---|----------------------|-------------|
| 1 | string | Full name and path of the UObject |

- **Example:**
    ```lua
    local Engine = FindFirstOf("Engine")
    print(string.format("Engine Name: %s", Engine:GetFullName()))

    -- Output
    -- Engine Name: FGGameEngine /Engine/Transient.FGGameEngine_2147482618
    ```

### GetFName()

- **Returns:** the FName of the UObject. This is equivalent to `Object->NamePrivate` in Unreal.
> Warning: All FNames returned by `__index` are returned by reference.

- **Return Value:** 

| # | Type                 | Information |
|---|----------------------|-------------|
| 1 | FName | FName of the UObject |

- **Example:**
    ```lua
    local Character = FindFirstOf("Character")
    if Character:IsValid() then
        local CharacterName = Character:GetFName()
        print(string.format("ComparisonIndex: 0x%X\n", CharacterName:GetComparisonIndex()))
    end
    ```

### GetAddress()

- **Returns:** where in memory the UObject is located.

- **Return Value:** 

| # | Type                 | Information |
|---|----------------------|-------------|
| 1 | integer | 64-bit integer, address of the UObject |

- **Example:**
    ```lua
    local Character = FindFirstOf("Character")
    if Character:IsValid() then
        print(string.format("Character: 0x%X\n", Character:GetAddress()))
    end
    ```

### GetClass()

- **Returns:** the class of this object. This is equivalent to `UObject->ClassPrivate` in Unreal.

- **Return Value:** 

| # | Type                 | Information |
|---|----------------------|-------------|
| 1 | UClass | The class of the UObject |

- **Example:**
    ```lua
    local Character = FindFirstOf("Character")
    if Character:IsValid() then
        print(string.format("Character Class: 0x%X\n", Character:GetClass():GetAddress()))
    end
    ```

### GetOuter()

- **Returns:** the outer of the UObject. This is equivalent to `Object->OuterPrivate` in Unreal.

- **Return Value:** 

| # | Type                 | Information |
|---|----------------------|-------------|
| 1 | UObject | The outer UObject of this UObject |

- **Example:**
    ```lua
    local Character = FindFirstOf("Character")
    if Character:IsValid() then
        print(string.format("Character Outer: 0x%X\n", Character:GetOuter():GetAddress()))
    end
    ```

### IsAnyClass()

- **Return type:** `bool`
- **Returns:** true if this `UObject` is a `UClass` or a derivative of `UClass`

### Reflection()

- **Return type:** `UObjectReflection`
- **Returns:** a reflection object

### GetPropertyValue(string MemberVariableName)

- **Return type:** `auto`
- Identical to `__index` metamethod (doing `UObject["ObjectMemberName"]`)

### SetPropertyValue(string MemberVariableName, auto NewValue)

- Identical to `__newindex` metamethod (doing `UObject["ObjectMemberName"] = NewValue`)

### IsClass()

- **Return type:** `bool`
- **Returns:** whether this object is a `UClass` or `UClass` derivative

### GetWorld()

- **Return type:** `UWorld`
- **Returns:** the `UWorld` that this `UObject` is contained within.

### IsA(UClass Class)

- **Return type:** `bool`
- **Returns:** whether this object is of the specified `UClass`.

### IsA(string FullClassName)

- **Return type:** `bool`
- **Returns:** whether this object is of the specified class name.

### HasAllFlags(EObjectFlags FlagsToCheck)

- **Return type:** `bool`
- **Returns:** whether the object has all of the specified flags.

### HasAnyFlags(EObjectFlags FlagsToCheck)

- **Return type:** `bool`
- **Returns:** whether the object has any of the specified flags.

### HasAnyInternalFlags(EInternalObjectFlags InternalFlagsToCheck)

- **Return type:** `bool`
- **Returns:** whether the object has any of the specified internal flags.

### CallFunction(UFunction Function, auto Params...)

- Calls the supplied `UFunction` on this `UObject`.

### ProcessConsoleExec(string Cmd, nil Reserved, UObject Executor)

- Calls `UObject::ProcessConsoleExec` with the supplied params.

### type()

- **Return type:** `string`
- **Returns:** the type of this object as known by UE4SS
- This does not return the type as known by Unreal
- Not equivalent to doing `type(UObject)`, which returns the type as known by Lua (a 'userdata')

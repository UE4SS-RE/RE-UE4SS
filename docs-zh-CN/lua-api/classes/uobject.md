# UObject

`UObject` 类是大多数虚幻引擎游戏对象所继承的基类。

## 继承
[RemoteObject](./remoteobject.md)

## 元方法

### __index

- **用法：** `UObject["ObjectMemberName"]` 或 `UObject.ObjectMemberName`

- 返回成员变量（反射属性或自定义属性）或一个 UFunction。

- 该方法可能返回任意类型；若返回的类型为非平凡类型，你可以对返回值使用 UObject 专属的 `type()` 函数来确定其类型。

- 若返回的类型为平凡类型，请使用 Lua 常规的 `type()` 函数。

- **返回值：**

| # | 类型 | 说明 |
|---|------|------|
| 1 | UObject 或 UFunction | 若类型为 `UObject`，实际类型可能是任何继承自 `UObject` 的类。 |

- **示例：**

    ```lua
    local Character = FindFirstOf("Character")

    -- 获取一个非平凡类型
    local MovementComponent = Character.CharacterMovement

    -- 获取一个平凡类型
    local JumpMaxCount = Character.JumpMaxCount

    -- 调用对象上的 UFunction 成员
    -- 记得调用时使用冒号（:）
    local CanCharacterJump = Character:CanJump()

    ```

### __newindex

- **用法：** `UObject["ObjectMemberName"] = NewValue` 或 `UObject.ObjectMemberName = NewValue`

- 把成员变量的值设为 `NewValue`。

- **示例：**
    设置内存中 `UEngine` 类第一个实例的 `MaxParticleResize` 的值。

    ```lua
    local Engine = FindFirstOf("Engine")
    Engine.MaxParticleResize = 4
    ```

### __eq

- **用法：** `ObjectA == ObjectB`

- 按底层 C++ 指针比较两个 `UObject` 实例。若两个 userdata 包装的是同一个远端 `UObject*`（或两者均为空/无效），返回 `true`，否则返回 `false`。与非 `UObject` 的 userdata 或其他类型比较时会安全地返回 `false`，不会崩溃。

- **示例：**

    ```lua
    local Player = FindFirstOf("BP_Player_C")
    if Player == Controller.Pawn then
        print("Pawn matches active player")
    end
    ```

## 方法

### GetFullName()

- **返回：** `UObject` 及其派生对象的完整名称与路径信息

- **返回值：**

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 该 UObject 的完整名称与路径 |

- **示例：**

    ```lua
    local Engine = FindFirstOf("Engine")
    print(string.format("Engine Name: %s", Engine:GetFullName()))

    -- 输出
    -- Engine Name: FGGameEngine /Engine/Transient.FGGameEngine_2147482618
    ```

### GetFName()

- **返回：** 该 UObject 的 FName。等价于虚幻引擎中的 `Object->NamePrivate`。

> 警告：通过 `__index` 返回的所有 FName 都是按引用返回的。

- **返回值：**

| # | 类型 | 说明 |
|---|------|------|
| 1 | FName | 该 UObject 的 FName |

- **示例：**

    ```lua
    local Character = FindFirstOf("Character")
    if Character:IsValid() then
        local CharacterName = Character:GetFName()
        print(string.format("ComparisonIndex: 0x%X\n", CharacterName:GetComparisonIndex()))
    end
    ```

### GetAddress()

- **返回：** 该 UObject 在内存中的位置。

- **返回值：**

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 64 位整数，该 UObject 的地址 |

- **示例：**

    ```lua
    local Character = FindFirstOf("Character")
    if Character:IsValid() then
        print(string.format("Character: 0x%X\n", Character:GetAddress()))
    end
    ```

### GetClass()

- **返回：** 该对象的类。等价于虚幻引擎中的 `UObject->ClassPrivate`。

- **返回值：**

| # | 类型 | 说明 |
|---|------|------|
| 1 | UClass | 该 UObject 的类 |

- **示例：**

    ```lua
    local Character = FindFirstOf("Character")
    if Character:IsValid() then
        print(string.format("Character Class: 0x%X\n", Character:GetClass():GetAddress()))
    end
    ```

### GetOuter()

- **返回：** 该 UObject 的 outer。等价于虚幻引擎中的 `Object->OuterPrivate`。

- **返回值：**

| # | 类型 | 说明 |
|---|------|------|
| 1 | UObject | 该 UObject 的 outer UObject |

- **示例：**

    ```lua
    local Character = FindFirstOf("Character")
    if Character:IsValid() then
        print(string.format("Character Outer: 0x%X\n", Character:GetOuter():GetAddress()))
    end
    ```

### IsAnyClass()

- **返回类型：** `bool`
- **返回：** 若该 `UObject` 是 `UClass` 或 `UClass` 的派生类则返回 true

### Reflection()

- **返回类型：** `UObjectReflection`
- **返回：** 一个反射对象

### GetPropertyValue(string MemberVariableName)

- **返回类型：** `auto`
- 与 `__index` 元方法（即 `UObject["ObjectMemberName"]`）相同

### SetPropertyValue(string MemberVariableName, auto NewValue)

- 与 `__newindex` 元方法（即 `UObject["ObjectMemberName"] = NewValue`）相同

### IsClass()

- **返回类型：** `bool`
- **返回：** 该对象是否为 `UClass` 或 `UClass` 的派生类

### GetWorld()

- **返回类型：** `UWorld`
- **返回：** 包含该 `UObject` 的 `UWorld`。

### IsA(UClass Class)

- **返回类型：** `bool`
- **返回：** 该对象是否为指定的 `UClass`。

### IsA(string FullClassName)

- **返回类型：** `bool`
- **返回：** 该对象是否为指定的类名。

### HasAllFlags(EObjectFlags FlagsToCheck)

- **返回类型：** `bool`
- **返回：** 该对象是否具有全部指定标志。

### HasAnyFlags(EObjectFlags FlagsToCheck)

- **返回类型：** `bool`
- **返回：** 该对象是否具有任意一个指定标志。

### HasAnyInternalFlags(EInternalObjectFlags InternalFlagsToCheck)

- **返回类型：** `bool`
- **返回：** 该对象是否具有任意一个指定的内部标志。

### CallFunction(UFunction Function, auto Params...)

- 在该 `UObject` 上调用指定的 `UFunction`。

### ProcessConsoleExec(string Cmd, nil Reserved, UObject Executor)

- 使用给定参数调用 `UObject::ProcessConsoleExec`。

### type()

- **返回类型：** `string`
- **返回：** UE4SS 所知的该对象类型
- 它返回的不是虚幻引擎所知的类型
- 不等于执行 `type(UObject)`，后者返回 Lua 所知的类型（即 'userdata'）

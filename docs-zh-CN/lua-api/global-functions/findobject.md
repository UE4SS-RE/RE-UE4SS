# FindObject

`FindObject` 是一个用于查找对象的函数。

重载 #1 按类名或对象短名称查找。

重载 #2 的行为与 [UE 源码](https://docs.unrealengine.com/4.27/en-US/API/Runtime/CoreUObject/UObject/FindObject/) 中的 `FindObject` 相同。

## 参数（重载 #1）

| # | 类型 | 说明 |
|---|------|------|
| 1 | string\|FName\|UClass\|nil | 对象所属类的短名称，或 UClass 本身 |
| 2 | string\|FName\|nil | 对象自身的短名称 |
| 3 | EObjectFlags | 对象不能具有的标志。使用 \| 分隔 |
| 4 | EObjectFlags | 对象必须具有的标志。使用 \| 分隔 |

> 参数 1 与参数 2 可以为 nil，但不能同时为 nil。

## 参数（重载 #2）

| # | 类型 | 说明 |
|---|------|------|
| 1 | UClass | 要查找的类 |
| 2 | UObject\|UClass | 要在其中查找的 outer。若为 null，则参数 3 应以包名开头 |
| 3 | string | 相对于参数 2 的对象路径 |
| 4 | bool | 是否要求与 UClass 参数精确匹配 |

## 返回值（重载 #1 与 #2）

| # | 类型 | 说明 |
|---|------|------|
| 1 | UObject | UObject 的派生对象 |

## 示例（重载 #1）

```lua
-- 名为 TransformComponent0 的 SceneComponent 实例
local Object = FindObject("SceneComponent", "TransformComponent0")
```

```lua
-- 名为 FirstPersonCharacter_C_0 的 FirstPersonCharacter_C 实例
local Object = FindObject("FirstPersonCharacter_C", "FirstPersonCharacter_C_0", EObjectFlags.RF_NoFlags, EObjectFlags.RF_ClassDefaultObject)
```

你也可以使用 UClass 对象：

```lua
local Object = FindObject(SceneComponentClass, "TransformComponent0")
```

## 示例（重载 #2）

```lua
local Object = FindObject(UClass, World, "Character", true)
```

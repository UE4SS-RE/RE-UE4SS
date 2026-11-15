# StaticFindObject

`StaticFindObject` 函数用于查找当前内存中任何继承自 `UObject` 的对象。

这是获取非实例对象（例如 UClass 或 UFunction 类型的对象）的推荐方式。

## 参数（重载 #1）

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 要查找的对象的完整名称，不含类型前缀 |

## 参数（重载 #2）

该重载的参数模仿 UE4 中的 `StaticFindObject` 函数。
更多信息见：[Unreal Engine API -> StaticFindObject](https://docs.unrealengine.com/4.27/en-US/API/Runtime/CoreUObject/UObject/StaticFindObject/)

| # | 类型 | 说明 |
|---|------|------|
| 1 | UClass | 要查找的对象所属的类，可以为 nil。 |
| 2 | UObject | 要在其中查找的 outer。为 nil 时会搜索所有包。 |
| 3 | string | 要查找的对象的名称 |
| 4 | bool | 是否要求与 UClass 参数精确匹配 |

## 返回值（重载 #1 与 #2）

| # | 类型 | 说明 |
|---|------|------|
| 1 | UObject、UClass 或 AActor | 只有当找到实例时该对象才有效 |

## 示例（重载 #1）

```lua
local CharacterInstance = StaticFindObject("/Script/Engine.Character")
if not CharacterInstance:IsValid() then
    print("No instance of class 'Character' was found.")
end
```

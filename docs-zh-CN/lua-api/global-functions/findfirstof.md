# FindFirstOf

`FindFirstOf` 函数会查找所给类名的第一个非默认实例。

> 该函数不能用于查找非实例对象或默认对象。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 要查找实例的类的短名称 |

## 返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | UObject、UClass 或 AActor | 只有当找到实例时该对象才有效 |

## 示例

```lua
local CharacterInstance = FindFirstOf("Character")
if not CharacterInstance:IsValid() then
    print("No instance of class 'Character' was found.")
end
```

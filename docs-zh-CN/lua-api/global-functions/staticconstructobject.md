# StaticConstructObject

`StaticConstructObject` 函数尝试构造一个指定类型的 UE4 对象。

该函数模仿 [StaticConstructObject_Internal](https://docs.unrealengine.com/4.27/en-US/API/Runtime/CoreUObject/UObject/StaticConstructObject_Internal/1/) 函数。

## 参数（重载 #1）

| # | 类型 | 说明 |
|---|------|------|
| 1 | UClass | 要构造的对象所属的类 |
| 2 | UObject | 要在其中构造对象的 outer |
| 3 | FName | 可选 |
| 4 | integer | 可选，64 位整数 |
| 5 | integer | 可选，64 位整数 |
| 6 | bool | 可选 |
| 7 | bool | 可选 |
| 8 | UObject | 可选 |
| 9 | integer | 可选，64 位整数 |
| 10 | integer | 可选，64 位整数 |
| 11 | integer | 可选，64 位整数 |

## 参数（重载 #2）

| # | 类型 | 说明 |
|---|------|------|
| 1 | UClass | 要构造的对象所属的类 |
| 2 | UObject | 要在其中构造对象的 outer |
| 3 | integer | 可选，一个 FName 的 64 位整数表示（ComparisonIndex 与 Number） |
| 4 | integer | 可选，64 位整数 |
| 5 | integer | 可选，64 位整数 |
| 6 | bool | 可选 |
| 7 | bool | 可选 |
| 8 | UObject | 可选 |
| 9 | integer | 可选，64 位整数 |
| 10 | integer | 可选，64 位整数 |
| 11 | integer | 可选，64 位整数 |

## 返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | UObject | 只有当对象成功构造时该对象才有效 |

## 示例

下面的示例构造一个 `UConsole` 对象。

```lua
local Engine = FindFirstOf("Engine")
local ConsoleClass = Engine.ConsoleClass
local GameViewport = Engine.GameViewport

if not ConsoleClass:IsValid() or not GameViewport:IsValid() then
    print("Was unable to construct UConsole because the console class didn't exist\n")
else
    local CreatedConsole = StaticConstructObject(ConsoleClass, GameViewport, 0, 0, 0, nil, false, false, nil)

    if CreatedConsole:IsValid() then
        print(string.format("CreatedConsole: %s\n", CreatedConsole:GetFullName()))
    else
        print("Was unable to construct UConsole\n")
    end
end
```

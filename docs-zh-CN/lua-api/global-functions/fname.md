# FName

`FName` 函数用于获取 `string` 或 `integer` 对应的 `FName` 表示。

## 参数（重载 #1）

该重载模仿 [FName::FName](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Core/UObject/FName/__ctor/7/)，其中 `FindType` 参数被设为 `EFindName::FName_Add`。

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 你希望获得其 FName 表示的字符串 |
| 2 | EFindName | 查找或添加名称的类型。可为 `FNAME_Find` 或 `FNAME_Add`。未显式传入时默认为 `FNAME_Add` |

## 参数（重载 #2）

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 代表 `ComparisonIndex` 的 64 位整数，你希望获得其 FName 表示 |
| 2 | EFindName | 查找或添加名称的类型。可为 `FNAME_Find` 或 `FNAME_Add`。未显式传入时默认为 `FNAME_Add` |

## 返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | FName | 与该字符串或 `ComparisonIndex` 对应的 FName（若存在）；若不存在则为 "None" 对应的 FName。若传入 `FNAME_Add`，则在名称不存在时添加它 |

## 示例

```lua
local name = FName("MyName")

print(name) -- MyName
```

# CreateInvalidObject

`CreateInvalidObject` 函数总是返回一个对象，其 `IsValid` 函数返回 `false`。

该函数的唯一目的，是确保 mod 的 Lua 代码符合 UE4SS 的代码约定：所有函数都返回一个无效的 `UObject` 而不是 `nil`。

## 示例

下面的示例代码让你永远不需要检查 `EngineCache` 是否为 `nil`，`GetEngine()` 的返回值同样如此。

```lua
local EngineCache = CreateInvalidObject() ---@cast EngineCache UEngine
---返回 UEngine 的实例
---@return UEngine
function GetEngine()
    if EngineCache:IsValid() then return EngineCache end

    EngineCache = FindFirstOf("Engine") ---@type UEngine
    return EngineCache
end
```

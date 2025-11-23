# CreateInvalidObject

The function `CreateInvalidObject` always returns an object with an `IsValid` function that returns `flase`.

The sole purpose of the function is to ensure that the mod's Lua code adheres to UE4SS code conventions, where all functions return an invalid `UObject` instead of `nil`.

## Example
The example code below ensures that you never need to check if `EngineCache` is `nil`, and the same applies to the return value of `GetEngine()`.
```lua
local EngineCache = CreateInvalidObject() ---@cast EngineCache UEngine
---Returns instance of UEngine
---@return UEngine
function GetEngine()
    if EngineCache:IsValid() then return EngineCache end

    EngineCache = FindFirstOf("Engine") ---@type UEngine
    return EngineCache
end
```
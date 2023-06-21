# LoadAsset

The `LoadAsset` function loads an asset by name.  

> It must only be called from within the game thread. For example, from within a `UFunction` hook or `RegisterConsoleCommandHandler` callback.  

## Parameters

| # | Type     | Information |
|---|----------|-------------|
| 1 | string   | Path and name of the asset |

## Example
```lua
RegisterConsoleCommandHandler("summon", function(FullCommand, Parameters)
    if #Parameters < 1 then return false end
    
    -- Parameters[1] example: /Game/LevelElements/Refinery/Pipeline/BP_Pipeline_Start
    LoadAsset(Parameters[1])

    return false
end)
```
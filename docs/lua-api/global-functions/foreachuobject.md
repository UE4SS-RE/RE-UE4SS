# ForEachUObject

The `ForEachUObject` function iterates every UObject that currently exists in `GUObjectArray`.

The `GUObjectArray` UE4 variable is a large chunked array that contains UObjects.  

The structure of this array has changed over the years and the `ForEachUObject` function is designed to work identically across all engine versions.

## Parameters

| # | Type     | Information |
|---|----------|-------------|
| 1 | function | Callback to execute for every UObject in GUObjectArray |

## Callback Parameters

| # | Type     | Information |
|---|----------|-------------|
| 1 | UObject  | The UObject |
| 2 | integer  | The chunk index of the UObject |
| 3 | integer  | The object index of the UObject |

## Example
```lua
-- Warning: This will take quite a while to finish executing due to all of the 'print' calls
ForEachUObject(function(Object, ChunkIndex, ObjectIndex)
    print(string.format("Chunk: %X | Object: %X | Name: %s\n", ChunkIndex, ObjectIndex, Object:GetFullName()))
end)
```
# LoadExport

The `LoadExport` function is used to load a symbol that's been exported by the game.

> [!IMPORTANT]
> This function doesn't handle undecorated symbol names.

## Parameters

| #      | Type  | Information             |
|--------|-------|-------------------------|
| string | any   | The name of the symbol. |

## Example
```lua
-- If the game exports GUObjectArray, like it does when UE is built modularly, this retrieves its address.
local GUObjectArray = LoadExport("?GUObjectArray@@3VFUObjectArray@@A")
```

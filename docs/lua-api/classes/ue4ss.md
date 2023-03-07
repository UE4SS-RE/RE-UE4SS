# UE4SS

The `UE4SS` class is for interacting with UE4SS metadata.

## Inheritance
None

## Methods

### GetVersion()

**Returns:** the current version of UE4SS that is being used.  
**Return Value:**

| # | Type    | Information |
|---|-------- |-------------|
| 1 | integer | Major version |
| 2 | integer | Minor version |
| 3 | integer | Hotfix version |

**Example #1**
> Warning: This only works in UE4SS 1.1+. See example #2 for UE4SS <=1.0.
```lua
local Major, Minor, Hotfix = UE4SS.GetVersion()
print(string.format("UE4SS v%d.%d.%d\n", Major, Minor, Hotfix))
```

**Example #2**

This example shows how to distinguish between UE4SS <=1.0, which didn't have the `UE4SS` class, and UE4SS >=1.1.
```lua
if UE4SS == nil then
    print("Running UE4SS <=1.0\n")
end
```

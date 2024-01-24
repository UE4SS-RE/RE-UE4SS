# FName

The `FName` function is used to get an `FName` representation of a `string` or `integer`.

## Parameters (overload #1)

This overload mimics [FName::FName](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Core/UObject/FName/__ctor/7/) with the `FindType` param set to `EFindName::FName_Add`.

| # | Type     | Information                                                                                                                  |
|---|----------|------------------------------------------------------------------------------------------------------------------------------|
| 1 | string   | String that you'd like to get an FName representation of                                                                     |
| 2 | EFindName | Finding or adding name type. It can be either `FNAME_Find` or `FNAME_Add`. Default is `FNAME_Add` if not explicitly supplied |

## Parameters (overload #2)

| # | Type     | Information                                                                                                                  |
|---|----------|------------------------------------------------------------------------------------------------------------------------------|
| 1 | integer  | 64-bit integer representing the `ComparisonIndex` part that you'd like to get an FName representation of                     |
| 2 | EFindName | Finding or adding name type. It can be either `FNAME_Find` or `FNAME_Add`. Default is `FNAME_Add` if not explicitly supplied |

## Return Value

| # | Type  | Information |
|---|-------|-------------|
| 1 | FName | FName corresponding to the string or `ComparisonIndex`, if one exists, or the "None" FName if one doesn't exist. If `FNAME_Add` is supplied then it adds the name if it doesn't exist |

## Example

```lua
local name = FName("MyName")

print(name) -- MyName
```
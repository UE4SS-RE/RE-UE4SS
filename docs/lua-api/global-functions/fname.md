# FName

The `FName` function is used to get an `FName` representation of a `string` or `integer`.

## Parameters (overload #1)

This overload mimics [FName::FName](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Core/UObject/FName/__ctor/7/) with the `FindType` param set to `EFindName::FName_Find`.

| # | Type     | Information |
|---|----------|-------------|
| 1 | string   | String that you'd like to get an FName representation of |

## Parameters (overload #2)

| # | Type     | Information |
|---|----------|-------------|
| 1 | integer  | 64-bit integer, covering both 'ComparisonIndex' and 'Number', that you'd like to get an FName representation of |

## Return Value

| # | Type  | Information |
|---|-------|-------------|
| 1 | FName | FName corresponding to the string or ComparisonIndex & Number combo, if one exists, or the "None" FName if one doesn't exist |
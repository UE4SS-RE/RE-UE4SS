# FText

The `FText` function is used to get an `FText` representation of a `string`.

Useful when you have to interact with `UserWidget`-related classes for the UI of your mods, and call their `SetText(FText("My New Text"))` methods.

## Parameters (overload #1)

This overload mimics [FText::FText( FString&& InSourceString )](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Core/Internationalization/FText/__ctor/6/).

| # | Type     | Information |
|---|----------|-------------|
| 1 | string   | String that you'd like to get an FText representation of |

## Return Value

| # | Type  | Information |
|---|-------|-------------|
| 1 | FText | FText representation of incoming `string`|

## Example

```lua
local some_text = FText("MyText")

print(some_text) -- MyText
```
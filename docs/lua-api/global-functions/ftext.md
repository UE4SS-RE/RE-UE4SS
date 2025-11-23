# FText

The `FText` function is used to create an `FText` object from a `string`.

Useful when you have to interact with `UserWidget`-related classes for the UI of your mods, and call their `SetText(FText("My New Text"))` methods.

## Parameters (overload #1)

This overload uses [FText::FText( FString&& InSourceString )](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Core/Internationalization/FText/__ctor/6/) to create a new `FText` object.

 # | Type     | Information 
---|----------|-------------
 1 | string   | Content with which FText will to be created

## Return Value

 # | Type  | Information 
---|-------|-------------
 1 | FText | FText object that contains the passed `string`

## Example
Code:
```lua
local my_text = FText("My Text")
print(string.format("Lua type: %s\n", type(my_text)))
print(string.format("Object type: %s\n", my_text:type()))
print(string.format("Content: %s\n", my_text:ToString()))
```
Output:
```
[Lua] Lua type: userdata
[Lua] Object type: FText
[Lua] Content: My Text
```
# StructPropertyInfo

The `StructPropertyInfo` table contains type information for custom StructProperty properties.

**You must supply data yourself when using this table.**

## Structure
| Key            | Value Type             | Information      |
|----------------|------------------------|---------------|
| Name           | string                 | Full name of `UScriptStruct` to use for the property. |

## Example
```lua
local StructPropertyInfo = {
    ["Name"] = "/Script/MyGame.MyStructType"
}
```

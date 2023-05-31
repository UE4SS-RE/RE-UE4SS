# OffsetInternalInfo

The `OffsetInternalInfo` table contains information related to a custom property.

**You must supply data yourself when using this table.**

## Structure
| Key            | Value Type     | Information |
|----------------|----------------|-------------|
| Property       | string         | Name of the property to use as relative start instead of base |
| RelativeOffset | integer        | Offset from relative start to this property |

## Example
```lua
local PropertyInfo = {
    ["Property"] = "HistoryBuffer",
    ["RelativeOffset"] = 0x10
}
```
# ArrayPropertyInfo

The `ArrayPropertyInfo` table contains type information for custom ArrayProperty properties.

**You must supply data yourself when using this table.**

## Structure
| Key            | Value Type             | Sub Type      |
|----------------|------------------------|---------------|
| Type           | table                  | [PropertyTypes](./propertytypes.md) |

## Example
```lua
local ArrayPropertyInfo = {
    ["Type"] = PropertyTypes.IntProperty
}
```
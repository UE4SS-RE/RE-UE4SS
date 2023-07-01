# UScriptStruct

## Inheritance
[LocalObject](./localobject.md)

## Methods

### __index(string StructMemberVarName) 

- **Return type:** `auto`
- **Returns:** the value for the supplied variable
- Can return any type, you can use the `type()` function on the returned value to figure out what Lua class it's using (if non-trivial type)
> Note: `__index` is a Lua [metamethod](https://gist.github.com/oatmealine/655c9e64599d0f0dd47687c1186de99f#indexing) and allows the access operation `userdata[key]`

- **Example:**
```lua
local scriptStruct = FindFirstOf('_UI_Items_C')

-- Either of the following can be used:
local item = scriptStruct['Item']
local item = scriptStruct.Item
```

### __newindex(string StructMemberVarName, auto NewValue)

- Attempts to set the value for the supplied variable
> Note: `__newindex` is a Lua [metamethod](https://gist.github.com/oatmealine/655c9e64599d0f0dd47687c1186de99f#indexing) and is an indexing assignment of `userdata[key] = value`

- **Example:**
```lua
local scriptStruct = FindFirstOf('_UI_Items_C')

-- Either of the following can be used:
scriptStruct['Item'] = 5
scriptStruct.Item = 5
```

### GetBaseAddress()

- **Return type:** `integer`
- **Returns:** the address in memory where the `UObject` that this `UScriptStruct` belongs to is located

### GetStructAddress()

- **Return type:** `integer`
- **Returns:** the address in memory where this `UScriptStruct` is located

### GetPropertyAddress()

- **Return type:** `integer`
- **Returns:** the address in memory where the corresponding `UProperty`/`FProperty` is located

### IsValid()

- **Return type:** `bool`
- **Returns:** whether the struct is valid

### IsMappedToObject()

- **Return type:** `bool`
- **Returns:** whether the base object is valid

### IsMappedToProperty()

- **Return type:** `bool`
- **Returns:**  whether the property is valid

### type()

- **Return type:** `string`
- **Returns:** "UScriptStruct"

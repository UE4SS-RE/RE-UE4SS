# UScriptStruct

## Inheritance
[LocalObject](./localobject.md)

## Methods

### __index(string StructMemberVarName) 

- **Return type:** `auto`
- **Returns:** the value for the supplied variable
- Can return any type, you can use the `type()` function on the returned value to figure out what Lua class it's using (if non-trivial type)

### __newindex(string StructMemberVarName, auto NewValue)

- Attempts to set the value for the supplied variable

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
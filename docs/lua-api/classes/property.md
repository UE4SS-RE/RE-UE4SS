# Property

## Inheritance
[RemoteObject](./remoteobject.md)

## Methods

### GetFullName()

- **Return type:** `string`
- **Returns:** the full name & path for this property.

### GetFName()

- **Return type:** `FName`
- **Returns:** the FName of this property by copy.
> All FNames returned by `__index` are returned by reference.

### IsA(PropertyTypes PropertyType)

- **Return type:** `bool`
- **Returns:** `true` if the property is of type `PropertyType`.

### GetClass()

- **Return type:** `PropertyClass`

### ContainerPtrToValuePtr(UObjectDerivative Container, integer ArrayIndex)

- **Return type:** `LightUserdata`
- Equivalent to `FProperty::ContainerPtrToValuePtr<uint8>` in UE.

### ImportText(string Buffer, LightUserdata Data, integer PortFlags, UObject OwnerObject)

- Equivalent to `FProperty::ImportText` in UE, except without the `ErrorText` param.
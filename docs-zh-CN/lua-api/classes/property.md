# Property

## 继承
[RemoteObject](./remoteobject.md)

## 方法

### GetFullName()

- **返回类型：** `string`
- **返回：** 该属性的完整名称与路径。

### GetFName()

- **返回类型：** `FName`
- **返回：** 该属性的 FName（按值复制）。

> 通过 `__index` 返回的所有 FName 都是按引用返回的。

### IsA(PropertyTypes PropertyType)

- **返回类型：** `bool`
- **返回：** 若该属性类型为 `PropertyType` 则为 `true`。

### GetClass()

- **返回类型：** `PropertyClass`

### ContainerPtrToValuePtr(UObjectDerivative Container, integer ArrayIndex)

- **返回类型：** `LightUserdata`
- 等价于 UE 中的 `FProperty::ContainerPtrToValuePtr<uint8>`。

### ImportText(string Buffer, LightUserdata Data, integer PortFlags, UObject OwnerObject)

- 等价于 UE 中的 `FProperty::ImportText`，只是没有 `ErrorText` 参数。

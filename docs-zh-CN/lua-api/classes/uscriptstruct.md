# UScriptStruct

## 继承
[LocalObject](./localobject.md)

## 元方法

### __index

- **用法：** `UScriptStruct["StructMemberName"]` 或 `UScriptStruct.StructMemberName`
- **返回类型：** `auto`
- 返回所给成员名称对应的值。
- 可能返回任意类型；你可以对返回值使用 `type()` 函数来确定它使用的 Lua 类（若为非平凡类型）。

- **示例：**

```lua
local scriptStruct = FindFirstOf('_UI_Items_C')

-- 以下两种写法均可：
local item = scriptStruct['Item']
local item = scriptStruct.Item
```

### __newindex

- **用法：** `UScriptStruct["StructMemberName"] = NewValue` 或 `UScriptStruct.StructMemberName = NewValue`
- 尝试把所给成员名称的值设为 `NewValue`。

- **示例：**

```lua
local scriptStruct = FindFirstOf('_UI_Items_C')

-- 以下两种写法均可：
scriptStruct['Item'] = 5
scriptStruct.Item = 5
```

## 方法

### GetBaseAddress()

- **返回类型：** `integer`
- **返回：** 该 `UScriptStruct` 所属的 `UObject` 所在的内存地址

### GetStructAddress()

- **返回类型：** `integer`
- **返回：** 该 `UScriptStruct` 所在的内存地址

### GetPropertyAddress()

- **返回类型：** `integer`
- **返回：** 对应 `UProperty`/`FProperty` 所在的内存地址

### IsValid()

- **返回类型：** `bool`
- **返回：** 该结构体是否有效

### IsMappedToObject()

- **返回类型：** `bool`
- **返回：** 其基础对象是否有效

### IsMappedToProperty()

- **返回类型：** `bool`
- **返回：** 其属性是否有效

### type()

- **返回类型：** `string`
- **返回：** "UScriptStruct"

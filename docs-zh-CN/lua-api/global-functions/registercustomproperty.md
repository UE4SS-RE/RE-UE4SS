# RegisterCustomProperty

`RegisterCustomProperty` 函数用于注册自定义属性，使其用起来就像已反射的原生属性或蓝图属性一样。

这是一个进阶函数，用于在多个元表的 `__index` 元方法中支持未反射的属性。

## 参数

| # | 类型 | 子类型 | 说明 |
|---|------|--------|------|
| 1 | table | [CustomPropertyInfo](https://github.com/UE4SS/UE4SS/wiki/Table%3A-CustomPropertyInfo) | 包含注册自定义属性所需全部信息的表 |

## 示例

下面的示例注册一个名为 `MySimpleCustomProperty` 的自定义属性，它把 `Character` 类任意实例中偏移 `0xF40` 处的数据当作 `IntProperty` 来访问。

随后它读取了 `Character` 类第一个实例的该值，用于演示这套机制如何工作。

```lua
RegisterCustomProperty({
    ["Name"] = "MySimpleCustomProperty",
    ["Type"] = PropertyTypes.IntProperty,
    ["BelongsToClass"] = "/Script/Engine.Character"
    ["OffsetInternal"] = 0xF40
})

local CharacterInstance = FindFirstOf("Character")
if CharacterInstance:IsValid() then
    local MySimplePropertyValue = CharacterInstance.MySimpleCustomProperty
end
```

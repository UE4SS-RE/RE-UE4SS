# UFunction

## 继承
[UStruct](./ustruct.md)

## 元方法

### __call

- **用法：** `UFunction(UFunctionParams...)`
- **返回类型：** `auto`
- 尝试调用该 `UFunction`，并返回其结果（若有）。
- 若获取 `UFunction` 时没有上下文（例如通过 `StaticFindObject`），则必须把 `UObject` 上下文作为第一个参数传入。

## 方法

### GetFunctionFlags()

- **返回类型：** `integer`
- **返回：** 该 `UFunction` 的标志。

### SetFunctionFlags(integer Flags)

- 设置该 `UFunction` 的标志。

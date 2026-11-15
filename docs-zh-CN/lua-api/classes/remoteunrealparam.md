# RemoteUnrealParam

这是适用于任意类型与类的动态包装器。

> 使用 Remote 还是 Local 变体取决于数据的需要，但两种参数类型的用法完全相同。

## 继承
[RemoteObject](./remoteobject.md)

## 方法

### Get() / get()

- **返回类型：** `auto`
- **返回：** 该参数底层的值。

## set(auto NewValue)

- 设置该参数底层的值。

## type()

- **返回类型：** `string`
- **返回：** "RemoteUnrealParam"。

## 继承类中被删除的方法

### IsValid()、GetAddress()

- **来自**：[RemoteObject](./remoteobject.md)
- **原因**：参数永远不可能是无效的，因此该函数没有意义。
- **技术细节**：缺少该函数是由于一个 bug，即 RemoteUnrealObject 未正确继承自 RemoteObject。
理论上修复它不会造成任何问题，但对重要的基类做根本性改动可能带来麻烦与维护负担，因此除非有充分理由，我们应避免这样做。

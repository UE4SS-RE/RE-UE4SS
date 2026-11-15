# RegisterCustomEvent

该函数注册一个回调，当某个以 `EventName` 命名的蓝图函数或事件被调用时，该回调会被执行。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 要挂钩的事件名称。 |
| 2 | function | 该事件被调用时要执行的回调。 |

## 示例

```lua
RegisterCustomEvent("MyCustomEvent", function()
    print("MyCustomEvent was called\n")
end)
```

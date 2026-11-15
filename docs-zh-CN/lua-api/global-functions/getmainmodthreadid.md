# GetMainModThreadId

`GetMainModThreadId` 函数可以获取执行 `main.lua` 的线程 id。

## 返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | [ThreadId](../classes/threadid.md) | |

## 示例

```lua
print(string.format("Main Thread Id: %s\n", GetMainModThreadId():ToString()))
```

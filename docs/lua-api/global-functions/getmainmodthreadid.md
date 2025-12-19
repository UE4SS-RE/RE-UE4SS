# GetMainModThreadId

`GetMainModThreadId` is a function that allows you to get the id for the thread that executes main.lua.

## Return Value

| # | Type                               | Information |
|---|------------------------------------|-------------|
| 1 | [ThreadId](../classes/threadid.md) |             |

## Example

```lua
print(string.format("Main Thread Id: %s\n", GetMainModThreadId():ToString()))
```

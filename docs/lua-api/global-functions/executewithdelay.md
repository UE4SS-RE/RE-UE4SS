# ExecuteWithDelay

The `ExecuteWithDelay` function asynchronously executes the supplied callback after the supplied delay is over.

## Parameters

| # | Type     | Information |
|---|----------|-------------|
| 1 | integer  | Delay, in milliseconds, to wait before executing the supplied callback |
| 2 | function | The callback to execute after the supplied delay is over |

## Example
```lua
ExecuteWithDelay(2000, function()
    print("Executed asynchronously after a 2 second delay\n")
end)
```
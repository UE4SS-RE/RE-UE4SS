# ExecuteAsync

The `ExecuteAsync` function asynchronously executes the supplied callback.  

It works in a similar manner to [ExecuteWithDelay](./executewithdelay.md), except that there is no delay beyond the cost of registering the callback.

## Parameters

| # | Type     | Information |
|---|----------|-------------|
| 1 | function | The callback to execute|

## Example
```lua
ExecuteAsync(function()
    print("Executed asynchronously\n")
end)
```
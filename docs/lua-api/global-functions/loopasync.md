# LoopAsync

Starts a loop that sleeps for the supplied number of milliseconds and stops when the callback returns true.

## Parameters

| # | Type | Information |
|---|------|-------------|
| 1 | integer | The number of milliseconds to sleep |
| 2 | function | The callback function |

## Example

```lua
LoopAsync(1000, function()
    print("Hello World!")
    return false -- Loops forever
end)
```
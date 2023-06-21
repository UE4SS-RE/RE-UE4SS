# RegisterCustomEvent

This registers a callback that will get called when a blueprint function or event is called with the name `EventName`.

## Parameters

| # | Type     | Information |
|---|----------|-------------|
| 1 | string   | Name of the event to hook. |
| 2 | function | The callback to call when the event is called. |

## Example

```lua
RegisterCustomEvent("MyCustomEvent", function()
    print("MyCustomEvent was called\n")
end)
```
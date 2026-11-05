# NotifyOnNewObject

`NotifyOnNewObject` 函数会在 UE4 通过 `StaticConstructObject_Internal` 构造出所给类的实例时执行回调。

该函数会考虑继承关系，因此若你传入 `"/Script/Engine.Actor"`，那么任何构造出来的 `AActor` 或其派生类的对象都会触发回调。

回调可以返回 `true`，以移除（注销）该特定的 `NotifyOnNewObject` 回调。

> 所给的类不需要在调用该函数之前就已存在。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 要监听实例构造的类的完整名称，不含类型前缀 |
| 2 | function | 构造出所给类的实例时要执行的回调 |

## 回调参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | UObject | 被构造出来的对象 |

## 回调返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | boolean | 若为 true，则移除当前回调，不再接收后续通知 |

## 示例

```lua
NotifyOnNewObject("/Script/Engine.World", function(ConstructedObject)
    print(string.format("World Constructed: %s\n", ConstructedObject:GetFullName()))
end)

NotifyOnNewObject("/Script/Engine.Actor", function(ConstructedObject)
    print(string.format("Actor Constructed: %s\n", ConstructedObject:GetFullName()))

    -- 找到第一个 actor 后注销该回调，
    -- 也就是说当前这个函数只会被调用一次
    return true
end)
```

## 不要这样做

请不要对同一个类重复多次调用 `NotifyOnNewObject`，如果有多个 mod 都这样做，可能造成性能问题（实际中已经出现过这种情况）。

例如下面这样：

```lua
NotifyOnNewObject("/Script/Engine.PlayerController", function(PlayerController)
    PlayerController.bShowMouseCursor = true
end)
NotifyOnNewObject("/Script/Engine.PlayerController", function(PlayerController)
    PlayerController.bForceFeedbackEnabled = false
end)
NotifyOnNewObject("/Script/Engine.PlayerController", function(PlayerController)
    PlayerController.InputYawScale = 2.5
end)
NotifyOnNewObject("/Script/Engine.PlayerController", function(PlayerController)
    PlayerController.InputPitchScale = -2.5
end)
NotifyOnNewObject("/Script/Engine.PlayerController", function(PlayerController)
    PlayerController.InputRollScale = 1.0
end)
```

应该写成这样：

```lua
NotifyOnNewObject("/Script/Engine.PlayerController", function(PlayerController)
    PlayerController.bShowMouseCursor = true
    PlayerController.bForceFeedbackEnabled = false
    PlayerController.InputYawScale = 2.5
    PlayerController.InputPitchScale = -2.5
    PlayerController.InputRollScale = 1.0
end)
```

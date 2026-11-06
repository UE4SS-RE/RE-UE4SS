# RegisterConsoleCommandGlobalHandler

`RegisterConsoleCommandGlobalHandler` 函数会在向 UE 控制台输入指定的自定义命令时，执行所提供的 Lua 函数。

与 `RegisterConsoleCommandHandler` 不同，这个全局版本会在所有上下文中执行回调。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 自定义命令的名称 |
| 2 | function | 向 UE 控制台输入该自定义命令时要执行的回调 |

## 回调参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 完整命令字符串 |
| 2 | table | 参数表（不含命令名） |
| 3 | FOutputDevice | 用于写入的输出设备 |

## 回调返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | bool | 是否阻止其他处理器处理该命令 |

## 示例

```lua
RegisterConsoleCommandGlobalHandler("CommandExample", function(FullCommand, Parameters, OutputDevice)
    print("RegisterConsoleCommandGlobalHandler:\n")

    print(string.format("Command: %s\n", FullCommand))
    print(string.format("Number of parameters: %i\n", #Parameters))

    for ParameterNumber, Parameter in ipairs(Parameters) do
        print(string.format("Parameter #%i -> '%s'\n", ParameterNumber, Parameter))
    end

    OutputDevice:Log("Write something to game console")

    return false
end)

-- 在控制台输入：CommandExample param1 "param 2" 3
```

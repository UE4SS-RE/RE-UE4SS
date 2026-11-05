# UE4SS

`UE4SS` 类用于访问 UE4SS 的元数据。

## 继承
无

## 方法

### GetVersion()

**返回：** 当前正在使用的 UE4SS 版本。
**返回值：**

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 主版本号 |
| 2 | integer | 次版本号 |
| 3 | integer | 热修复版本号 |

**示例 #1**

> 警告：这只在 UE4SS 1.1+ 中可用。UE4SS <=1.0 请参考示例 #2。

```lua
local Major, Minor, Hotfix = UE4SS.GetVersion()
print(string.format("UE4SS v%d.%d.%d\n", Major, Minor, Hotfix))
```

**示例 #2**

这个示例展示了如何区分没有 `UE4SS` 类的 UE4SS <=1.0 与 UE4SS >=1.1。

```lua
if UE4SS == nil then
    print("Running UE4SS <=1.0\n")
end
```

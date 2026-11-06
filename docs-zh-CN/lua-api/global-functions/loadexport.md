# LoadExport

`LoadExport` 函数用于加载游戏导出的符号。

> [!IMPORTANT]
> 该函数不处理未修饰的符号名。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| string | any | 符号的名称。 |

## 示例

```lua
-- 如果游戏导出了 GUObjectArray（当 UE 以模块化方式构建时就是如此），这会获取其地址。
local GUObjectArray = LoadExport("?GUObjectArray@@3VFUObjectArray@@A")
```

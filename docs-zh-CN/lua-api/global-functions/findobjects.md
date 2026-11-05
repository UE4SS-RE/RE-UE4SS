# FindObjects

按类名或对象短名称查找指定数量的对象（取最先找到的若干个）。

若要查找符合条件的所有对象，请把参数 1 设为 `0` 或 `nil`。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 要查找的对象数量 |
| 1 | string\|FName\|nil | 对象所属类的短名称 |
| 2 | string\|FName\|nil | 对象自身的短名称 |
| 3 | EObjectFlags | 对象不能具有的标志。使用 \| 分隔 |
| 4 | EObjectFlags | 对象必须具有的标志。使用 \| 分隔 |
| 6 | bool | 是否要求与 UClass 参数精确匹配 |

## 返回值

| # | 类型 | 子类型 | 说明 |
|---|------|--------|------|
| 1 | table | UObject | UObject 的派生对象 |

## 示例

```lua
local Object = FindObjects(4, "SceneComponent", "TransformComponent0", EObjectFlags.RF_NoFlags, EObjectFlags.RF_ClassDefaultObject, true)

for _, Object in pairs(Objects) do
    -- 对 Object 做一些处理
end
```

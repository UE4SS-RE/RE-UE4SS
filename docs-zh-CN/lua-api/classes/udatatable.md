# UDataTable

## 继承
[UObject](./uobject.md)

## 元方法

### __len

- **用法：** `#UDataTable`
- **返回类型：** `integer`
- 返回数据表中的行数。

## 方法

### IsValid()

- **返回类型：** `bool`
- **返回：** 数据表是否有效（非空）。

### GetRowStruct()

- **返回类型：** `UScriptStruct` | `nil`
- **返回：** 定义该数据表行结构的 UScriptStruct；若未设置则返回 nil。

### GetRowMap()

- **返回类型：** `table`
- **返回：** 包含所有行的 Lua 表，行名作为键、行数据作为值。
- 每个值的数据类型取决于该行类型是否存在自定义 pusher：可能是 UScriptStruct，也可能是 Lua 表。

### FindRow(string RowName)

- **返回类型：** `UScriptStruct` | `nil`
- **返回：** 找到时返回作为 UScriptStruct 的行数据，若该行不存在则返回 nil。
- 返回的结构体提供基于引用的访问方式——修改会直接作用于 DataTable。
- 示例：`local row = dt:FindRow("Player"); row.Health = 200` 会修改实际的 DataTable。

### AddRow(string RowName, table|UScriptStruct RowData)

- 以指定的名称与数据向数据表添加一行。
- 若已存在同名行，则会被替换。
- 接受 Lua 表或 UScriptStruct：
  - **表：** 字段应与行结构体的定义相匹配
  - **UScriptStruct：** 从该结构体复制数据（适合克隆行）
- 示例：`dt:AddRow("NewPlayer", {Health = 100, MaxHealth = 100})`
- 示例：`dt:AddRow("Clone", dt:FindRow("Original"))` -- 复制一行

### RemoveRow(string RowName)

- 从数据表中移除指定名称的行。
- 若该行不存在则什么也不做。

### EmptyTable()

- 从数据表中移除所有行。
- 注意：不会清除 RowStruct 定义。

### GetRowNames()

- **返回类型：** `table`
- **返回：** 数据表中所有行名组成的数组（从 1 开始索引的表）。

### GetAllRows()

- **返回类型：** `table`
- **返回：** 一个数组（从 1 开始索引的表），每个元素都是包含两个字段的表：
  - `Name`：行名（string）
  - `Data`：行数据（table 或 UScriptStruct）

### ForEachRow(function Callback)

- 遍历数据表中的所有行，并对每一行调用回调函数。
- 回调参数为：`string rowName`、`UScriptStruct rowData`。
- rowData 参数提供基于引用的访问方式——修改会直接作用于 DataTable。
- 回调可以返回 `true` 以提前跳出循环。
- 示例：

```lua
dataTable:ForEachRow(function(rowName, rowData)
    print(string.format("Row: %s, Health: %d", rowName, rowData.Health))
    -- 直接修改该行
    rowData.Health = rowData.Health + 10
    -- 返回 true 以停止遍历
    if rowName == "TargetRow" then
        return true
    end
end)
```

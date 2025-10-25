# UDataTable

## Inheritance
[UObject](./uobject.md)

## Metamethods

### __len
- **Usage:** `#UDataTable`
- **Return type:** `integer`
- Returns the number of rows in the data table.

## Methods

### IsValid()
- **Return type:** `bool`
- **Returns:** whether the data table is valid (not null).

### GetRowStruct()
- **Return type:** `UScriptStruct` | `nil`
- **Returns:** the UScriptStruct that defines the row structure for this data table, or nil if not set.

### GetRowMap()
- **Return type:** `table`
- **Returns:** a Lua table containing all rows, with row names as keys and row data as values.
- Each value is either a UScriptStruct or a Lua table depending on whether a custom pusher exists for the row type.

### FindRow(string RowName)
- **Return type:** `UScriptStruct` | `nil`
- **Returns:** the row data as a UScriptStruct if found, or nil if the row doesn't exist.
- The returned struct provides reference-based access - modifications directly affect the DataTable.
- Example: `local row = dt:FindRow("Player"); row.Health = 200` modifies the actual DataTable.

### AddRow(string RowName, table|UScriptStruct RowData)
- Adds a new row to the data table with the specified name and data.
- If a row with the same name already exists, it will be replaced.
- Accepts either a Lua table or a UScriptStruct:
  - **Table:** Fields should match the row struct definition
  - **UScriptStruct:** Data is copied from the struct (useful for cloning rows)
- Example: `dt:AddRow("NewPlayer", {Health = 100, MaxHealth = 100})`
- Example: `dt:AddRow("Clone", dt:FindRow("Original"))` -- Copy a row

### RemoveRow(string RowName)
- Removes the row with the specified name from the data table.
- If the row doesn't exist, does nothing.

### EmptyTable()
- Removes all rows from the data table.
- Note: Does not clear the RowStruct definition.

### GetRowNames()
- **Return type:** `table`
- **Returns:** an array (1-indexed table) of all row names in the data table.

### GetAllRows()
- **Return type:** `table`
- **Returns:** an array (1-indexed table) where each element is a table with two fields:
  - `Name`: the row name (string)
  - `Data`: the row data (table or UScriptStruct)

### ForEachRow(function Callback)
- Iterates through all rows in the data table and calls the callback function for each row.
- The callback params are: `string rowName`, `UScriptStruct rowData`.
- The rowData parameter provides reference-based access - modifications directly affect the DataTable.
- The callback can optionally return `true` to break out of the loop early.
- Example:
```lua
dataTable:ForEachRow(function(rowName, rowData)
    print(string.format("Row: %s, Health: %d", rowName, rowData.Health))
    -- Modify the row directly
    rowData.Health = rowData.Health + 10
    -- Return true to stop iterating
    if rowName == "TargetRow" then
        return true
    end
end)
```

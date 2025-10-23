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
- Each value is either a struct wrapper or a Lua table depending on whether a custom pusher exists for the row type.

### FindRow(string RowName)
- **Return type:** `table` | `nil`
- **Returns:** the row data as a Lua table if found, or nil if the row doesn't exist.
- Note: Currently returns a copy of the row data. Modifications to the returned table do not affect the data table.

### AddRow(string RowName, table RowData)
- Adds a new row to the data table with the specified name and data.
- If a row with the same name already exists, it will be replaced.
- The RowData table should contain fields matching the row struct definition.

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
  - `Data`: the row data (table or struct wrapper)

### ForEachRow(function Callback)
- Iterates through all rows in the data table and calls the callback function for each row.
- The callback params are: `string rowName`, `table rowData` | `RemoteUnrealParam rowData`.
- The callback can optionally return `true` to break out of the loop early.
- Example:
```lua
dataTable:ForEachRow(function(rowName, rowData)
    print(string.format("Row: %s", rowName))
    -- Return true to stop iterating
    if rowName == "TargetRow" then
        return true
    end
end)
```

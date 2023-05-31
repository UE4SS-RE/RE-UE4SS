# IterateGameDirectories

Returns a table of all game directories.

An example of an absolute path to `Win64`: `Q:\SteamLibrary\steamapps\common\Deep Rock Galactic\FSD\Binaries\Win64`.

To get to the same directory, do `IterateGameDirectories().<Game Name>.Binaries.Win64`.

- You can use `.__name` and `.__absolute_path` to retrieve values.
- You can use `.__files` to retrieve a table containing all files in this directory.
- You also use `.__name` and `.__absolute_path` for files.

## Return Value

| # | Type | Information |
|---|------|-------------|
| 1 | table | The game directories table |

## Example

```lua
for _, GameDirectory in pairs(IterateGameDirectories()) do
    print(GameDirectory.__name)
    print(GameDirectory.__absolute_path)
end
```
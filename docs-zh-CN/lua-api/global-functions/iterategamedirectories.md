# IterateGameDirectories

返回包含所有游戏目录的表。

`Win64` 的绝对路径示例：`Q:\SteamLibrary\steamapps\common\Deep Rock Galactic\FSD\Binaries\Win64`。

要访问同一个目录，可以这样写：`IterateGameDirectories().<游戏名>.Binaries.Win64`。

- 你可以使用 `.__name` 与 `.__absolute_path` 获取对应的值。
- 你可以使用 `.__files` 获取包含该目录下所有文件的表。
- 对文件同样可以使用 `.__name` 与 `.__absolute_path`。

## 返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | table | 游戏目录表 |

## 示例

```lua
for _, GameDirectory in pairs(IterateGameDirectories()) do
    print(GameDirectory.__name)
    print(GameDirectory.__absolute_path)
end
```

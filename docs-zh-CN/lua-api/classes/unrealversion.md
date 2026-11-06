# UnrealVersion

`UnrealVersion` 类包含用于查询当前使用的虚幻引擎版本的辅助函数。

## 继承
无

## 方法

### GetMajor()

- **返回类型：** `integer`

### GetMinor()

- **返回类型：** `integer`

### IsEqual(number MajorVersion, number MinorVersion)

- **返回类型：** `bool`

### IsAtLeast(number MajorVersion, number MinorVersion)

- **返回类型：** `bool`

### IsAtMost(number MajorVersion, number MinorVersion)

- **返回类型：** `bool`

### IsBelow(number MajorVersion, number MinorVersion)

- **返回类型：** `bool`

### IsAbove(number MajorVersion, number MinorVersion)

- **返回类型：** `bool`

## 示例

```lua
local Major = UnrealVersion.GetMajor()
local Minor = UnrealVersion.GetMinor()
print(string.format("Version: %s.%s\n", Major, Minor))

if UnrealVersion.IsEqual(5, 0) then print("Version is 5.0\n") end
if UnrealVersion.IsAtLeast(5, 0) then print("Version is >=5.0\n") end
if UnrealVersion.IsAtMost(5, 0) then print("Version is <=5.0\n") end
if UnrealVersion.IsBelow(5, 0) then print("Version is <5.0\n") end
if UnrealVersion.IsAbove(5, 0) then print("Version is >5.0\n") end
```

# UnrealVersion

The `UnrealVersion` class contains helper functions for retrieving which version of Unreal Engine that is being used.

## Inheritance
None

## Methods

### GetMajor()

- **Return type:** `integer`

### GetMinor()

- **Return type:** `integer`

### IsEqual(number MajorVersion, number MinorVersion)

- **Return type:** `bool`

### IsAtLeast(number MajorVersion, number MinorVersion)

- **Return type:** `bool`

### IsAtMost(number MajorVersion, number MinorVersion)

- **Return type:** `bool`

### IsBelow(number MajorVersion, number MinorVersion)

- **Return type:** `bool`

### IsAbove(number MajorVersion, number MinorVersion)

- **Return type:** `bool`

## Examples
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

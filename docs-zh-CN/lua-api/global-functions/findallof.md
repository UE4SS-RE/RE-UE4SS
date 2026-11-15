# FindAllOf

`FindAllOf` 函数会查找所给类名的所有非默认实例。

> 该函数不能用于查找非实例对象或默认对象。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 要查找实例的类的短名称 |

## 返回值

| # | 类型 | 子类型 | 说明 |
|---|------|--------|------|
| 1 | nil 或 table | UObject、UClass 或 AActor | 若未找到实例则返回 nil，否则返回以数字索引包含所有实例的表 |

## 示例

输出所有继承自 `Actor` 类的对象名称。

```lua
local ActorInstances = FindAllOf("Actor")
if not ActorInstances then
    print("No instances of 'Actor' were found\n")
else
    for Index, ActorInstance in pairs(ActorInstances) do
        print(string.format("[%d] %s\n", Index, ActorInstance:GetFullName()))
    end
end
```

## 性能

该函数非常慢，因为它必须完整遍历一次整个全局对象数组。

在正式代码中应尽量避免使用它。

如果确实要用，可以用 `#` 运算符查看找到了多少个对象。

如果数量很多，请考虑在遍历这些对象时不要做太多工作。

与其使用 `UObject::GetFullName` 并处理字符串，不如手动遍历 outer、类以及/或者父类链，并对链上的每个元素调用 `UObject::GetFName`。

你可以把 FName 缓存起来，在循环外进行比较。

或者，改用 `UObject::IsA`。

下面的示例查找所有 actor，并只对 `Light` 的实例执行逻辑。
这个示例故意写得比较复杂，用以展示如何遍历父结构体链表。

```lua
local AllActors = FindAllOf("Actor")
print(string.format("NumActors found: %s\n", #AllActors))
local LightName = UEHelpers.FindOrAddFName("Light")
for _, Actor in ipairs(AllActors) do
    -- 按类型过滤。
    -- 类代表类型。
    local ActorClass = Actor:GetClass()

    -- 检查直接类型。
    local bIsLight = false
    if ActorClass:GetFName() == LightName then
        -- 直接就是 Light。
        bIsLight = true
    end

    -- 检查继承关系。
    local Super = ActorClass:GetSuperStruct()
    while Super:IsValid() do
        if Super:GetFName() == LightName then
            -- 继承自 Light。
            bIsLight = true
        end
        Super = Super:GetSuperStruct()
    end

    if bIsLight then
        -- 对 Light 执行逻辑。
    end
end
```

如果你给 `FindAllOf` 传入更具体的类型，这类过滤往往可以交给它完成。
例如，上面的示例完全可以替换为一次 `FindAllOf("Light")` 调用。

下面再看一个更有用的示例：我们想获取所有灯光，并根据灯光类型对它们执行不同操作：

```lua
local function IsObjectOfType(Object, TypeName)
    -- 按类型过滤。
    -- 类代表类型。
    local ObjectClass = Object:GetClass()

    -- 检查直接类型。
    if ObjectClass:GetFName() == TypeName then
        return true
    end

    -- 检查继承关系。
    local Super = ObjectClass:GetSuperStruct()
    while Super:IsValid() do
        if Super:GetFName() == TypeName then
            return true
        end
        Super = Super:GetSuperStruct()
    end

    return false
end

local AllActors = FindAllOf("Light")
print(string.format("NumActors found: %s\n", #AllActors))
local PointLightName = UEHelpers.FindOrAddFName("PointLight")
local RectLightName = UEHelpers.FindOrAddFName("RectLight")
local SpotLightName = UEHelpers.FindOrAddFName("SpotLight")
local DirectionalLightName = UEHelpers.FindOrAddFName("DirectionalLight")
for _, Light in ipairs(AllActors) do
    if IsObjectOfType(Light, PointLightName) then
        -- 对 PointLight 执行逻辑。
    end

    if IsObjectOfType(Light, RectLightName) then
        -- 对 RectLight 执行逻辑。
    end

    if IsObjectOfType(Light, SpotLightName) then
        -- 对 SpotLight 执行逻辑。
    end

    if IsObjectOfType(Light, DirectionalLightName) then
        -- 对 DirectionalLightName 执行逻辑。
    end
end
```

如果你不想自己写过滤器，也可以使用辅助函数 `IsA`。
`IsA` 函数会做完整的类型检查，并使用类对象而不是名称，因此你需要自行缓存并传入这些类对象。
这是验证类型最准确的方式。

```lua
local AllActors = FindAllOf("Light")
print(string.format("NumActors found: %s\n", #AllActors))
local PointLightClass = StaticFindObject("/Script/Engine.PointLight")
local RectLightClass = StaticFindObject("/Script/Engine.RectLight")
local SpotLightClass = StaticFindObject("/Script/Engine.SpotLight")
local DirectionalLightClass = StaticFindObject("/Script/Engine.DirectionalLight")
for _, Actor in ipairs(AllActors) do
    if Actor:IsA(PointLightClass) then
        -- 对 PointLight 执行逻辑。
        print(string.format("PL: '%s'\n", Actor:GetFullName()))
    end

    if Actor:IsA(RectLightClass) then
        -- 对 RectLight 执行逻辑。
        print(string.format("RL: '%s'\n", Actor:GetFullName()))
    end

    if Actor:IsA(SpotLightClass) then
        -- 对 SpotLight 执行逻辑。
        print(string.format("SL: '%s'\n", Actor:GetFullName()))
    end

    if Actor:IsA(DirectionalLightClass) then
        -- 对 DirectionalLightName 执行逻辑。
        print(string.format("DL: '%s'\n", Actor:GetFullName()))
    end
end
```

# FindAllOf

The `FindAllOf` function will find all non-default instances of the supplied class name.

> This function cannot be used to find non-instances or default instances.

## Parameters

| # | Type    | Information |
|---|---------|-------------|
| 1 | string  | Short name of the class to find instances of |

## Return Value

| # | Type         | Sub Type | Information |
|---|--------------|----------|-------------|
| 1 | nil or table | UObject, UClass, or AActor | nil if no instances were found, otherwise a numerically indexed table of all instances |

## Example
Outputs the name of all objects that inherit from the `Actor` class.
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

## Performance

This function is very slow as it must complete an entire pass of the entire global object array.

Avoid using this function in production code when avoidable.

If you must use it, you can use the `#` operator to check how many objects were found.

If there are a lot, consider not doing a lot of work while iterating these objects.

Prefer manually iterating the outer, class, and/or super chain(s) and calling `UObject::GetFName` on each element in the chain over using `UObject::GetFullName` and processing the string.

You can cache FNames to compare against outside your loop.

Alternatively, call `UObject::IsA` instead.

Here's an example of finding all actors, and filtering to only do logic on instances of `Light`.  
This example is intentionally complicated, and is meant to showcase how iterating the super struct linked list works.

```lua
local AllActors = FindAllOf("Actor")
print(string.format("NumActors found: %s\n", #AllActors))
local LightName = UEHelpers.FindOrAddFName("Light")
for _, Actor in ipairs(AllActors) do
    -- Filter on the type.
    -- The class represents the type.
    local ActorClass = Actor:GetClass()

    -- Check direct type.
    local bIsLight = false
    if ActorClass:GetFName() == LightName then
        -- Is directly a Light.
        bIsLight = true
    end

    -- Check inheritance.
    local Super = ActorClass:GetSuperStruct()
    while Super:IsValid() do
        if Super:GetFName() == LightName then
            -- Inherits from Light.
            bIsLight = true
        end
        Super = Super:GetSuperStruct()
    end

    if bIsLight then
        -- Do logic on Light.
    end
end
```

This filtering can often be done for you if you specify a lower type to the `FindAllOf` function.  
For example, the above example could be replaced entirely with a call to `FindAllOf("Light")`.

Here's another more useful example where we want to get all lights, and do operations on them depending on what type of light it is:

```lua
local function IsObjectOfType(Object, TypeName)
    -- Filter on the type.
    -- The class represents the type.
    local ObjectClass = Object:GetClass()

    -- Check direct type.
    if ObjectClass:GetFName() == TypeName then
        return true
    end

    -- Check inheritance.
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
        -- Do logic on PointLight.
    end

    if IsObjectOfType(Light, RectLightName) then
        -- Do logic on RectLight.
    end

    if IsObjectOfType(Light, SpotLightName) then
        -- Do logic on SpotLight.
    end

    if IsObjectOfType(Light, DirectionalLightName) then
        -- Do logic on DirectionalLightName.
    end
end
```

You can also make use of the helper function `IsA` if you'd rather not write your own filter.  
The `IsA` function does a full type check and uses class objects instead of names, which is why you need to
cache and supply them yourself.  
It's the most accurate way to verify types.

```lua
local AllActors = FindAllOf("Light")
print(string.format("NumActors found: %s\n", #AllActors))
local PointLightClass = StaticFindObject("/Script/Engine.PointLight")
local RectLightClass = StaticFindObject("/Script/Engine.RectLight")
local SpotLightClass = StaticFindObject("/Script/Engine.SpotLight")
local DirectionalLightClass = StaticFindObject("/Script/Engine.DirectionalLight")
for _, Actor in ipairs(AllActors) do
    if Actor:IsA(PointLightClass) then
        -- Do logic on PointLight.
        print(string.format("PL: '%s'\n", Actor:GetFullName()))
    end

    if Actor:IsA(RectLightClass) then
        -- Do logic on RectLight.
        print(string.format("RL: '%s'\n", Actor:GetFullName()))
    end

    if Actor:IsA(SpotLightClass) then
        -- Do logic on SpotLight.
        print(string.format("SL: '%s'\n", Actor:GetFullName()))
    end

    if Actor:IsA(DirectionalLightClass) then
        -- Do logic on DirectionalLightName.
        print(string.format("DL: '%s'\n", Actor:GetFullName()))
    end
end
```

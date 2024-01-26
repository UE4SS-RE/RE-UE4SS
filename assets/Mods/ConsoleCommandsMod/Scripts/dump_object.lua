-- This is a custom implementation of UEs "obj dump <obj>" command.
-- It doesn't have a 1:1 feature set.

local UEHelpers = require("UEHelpers")

--[[
    TODO: Expand on the console command format.
          You should be able to specify a sub-object, sub-struct, sub-array, sub-map, etc.
          You do 'dump_object /Engine/Transient.GameEngine_2147482624'.
          You notice '(ArrayProperty) ShaderComplexityColors=StructProperty ...'.
          To get the first element, you do: 'dump_object /Engine/Transient.GameEngine_2147482624 ShaderComplexityColors 0'.
          This will likely see the type-prefix (i.e. UClass /Engine/Script/...) needing to be unsupported just to simplify the code.
          UPDATE: This is currently working with ArrayProperty and StructProperty.
                  Need to add support eventually for MapProperty when UE4SS Lua supports MapProperty.
]]

string.padleft = function(InString, Length, PadWithChar)
    -- We pad with spaces by default.
    if not PadWithChar then PadWithChar = " " end
    return InString .. PadWithChar:rep(Length - #InString)
end

local OriginalLog = Log
local Buffer = ""
function Log(Message)
    Buffer = string.format("%s%s\n", Buffer, Message)
    OriginalLog(Message)
end

local UClassStaticClass = StaticFindObject("/Script/CoreUObject.Class")
local UScriptStructStaticClass = StaticFindObject("/Script/CoreUObject.ScriptStruct")
local UEnumStaticClass = StaticFindObject("/Script/CoreUObject.Enum")
local UPropertyStaticClass = StaticFindObject("/Script/CoreUObject.Property")

local DumpPropertyWithinObject = nil
local DumpObject = nil

DumpPropertyWithinObject = function(Object, Property, DumpRecursively)
    local ValueStr = ""

    -- Cannot resolve the value here because of unhandled types.
    if Property:IsA(PropertyTypes.Int8Property) then
        local Value = Object[Property:GetFName():ToString()]
        ValueStr = string.format("%s", Value)
    elseif Property:IsA(PropertyTypes.Int16Property) then
        local Value = Object[Property:GetFName():ToString()]
        ValueStr = string.format("%s", Value)
    elseif Property:IsA(PropertyTypes.IntProperty) then
        local Value = Object[Property:GetFName():ToString()]
        ValueStr = string.format("%s", Value)
    elseif Property:IsA(PropertyTypes.Int64Property) then
        local Value = Object[Property:GetFName():ToString()]
        ValueStr = string.format("%s", Value)
    elseif Property:IsA(PropertyTypes.NameProperty) then
        local Value = Object[Property:GetFName():ToString()]
        ValueStr = string.format("%s", Value:ToString())
    elseif Property:IsA(PropertyTypes.FloatProperty) then
        local Value = Object[Property:GetFName():ToString()]
        ValueStr = string.format("%s", Value)
    elseif Property:IsA(PropertyTypes.StrProperty) then
        local Value = Object[Property:GetFName():ToString()]
        ValueStr = string.format("%s", Value:ToString())
    elseif Property:IsA(PropertyTypes.ByteProperty) then
        local Value = Object[Property:GetFName():ToString()]
        ValueStr = string.format("%s", Value)
    elseif Property:IsA(PropertyTypes.ArrayProperty) then
        local Value = Object[Property:GetFName():ToString()]
        local Inner = Property:GetInner()

        -- Inner Type
        if Inner:IsA(PropertyTypes.StructProperty) then
            ValueStr = string.format("%s\n", Inner:GetStruct():GetFullName())
        else
            ValueStr = string.format("%s\n", Inner:GetFullName())
        end
        
        -- Size & Capacity
        if Value:GetArrayNum() == 0 then
            ValueStr = string.format("%s    ---empty---", ValueStr)
        else
            ValueStr = string.format("%s    %i/%i", ValueStr, Value:GetArrayNum(), Value:GetArrayMax())
        end
        
        -- Inner Value
        if Value:GetArrayNum() > 0 then
            ValueStr = string.format("%s\n", ValueStr)
            if Inner:IsA(PropertyTypes.ObjectProperty) then
                Value:ForEach(function(Index, Elem)
                    ValueStr = string.format("%s    [%i]: %s\n", ValueStr, Index - 1, Elem:get():GetFullName())
                end)
            end
        end
    elseif Property:IsA(PropertyTypes.MapProperty) then
        ValueStr = "UNHANDLED_VALUE"
    elseif Property:IsA(PropertyTypes.StructProperty) then
        local Value = Object[Property:GetFName():ToString()]
        ValueStr = string.format("%s", Value:GetFullName())
        if DumpRecursively then
            DumpObject(Value)
        end
    elseif Property:IsA(PropertyTypes.ClassProperty) then
        local Value = Object[Property:GetFName():ToString()]
        ValueStr = string.format("%s", Value:GetFullName())
    elseif Property:IsA(PropertyTypes.WeakObjectProperty) then
        --local Value = Object[Property:GetFName():ToString()]
        ValueStr = "UNHANDLED_VALUE"
    elseif Property:IsA(PropertyTypes.EnumProperty) then
        local Value = Object[Property:GetFName():ToString()]
        ValueStr = string.format("%s(%s)", Property:GetEnum():GetNameByValue(Value):ToString(), Value)
    elseif Property:IsA(PropertyTypes.TextProperty) then
        local Value = Object[Property:GetFName():ToString()]
        ValueStr = string.format("%s", Value:ToString())
    elseif Property:IsA(PropertyTypes.ObjectProperty) then
        local Value = Object[Property:GetFName():ToString()]
        if DumpRecursively then
            DumpObject(Value)
        else
            ValueStr = string.format("%s", Value:GetFullName())
        end
    elseif Property:IsA(PropertyTypes.BoolProperty) then
        local Value = Object[Property:GetFName():ToString()]
        local FieldMask = Property:GetFieldMask()
        if FieldMask == 255 then
            ValueStr = string.format("%s", (Value and "True" or "False"))
        else
            ValueStr = string.format("%s (FM: 0x%X, BM: 0x%X)", (Value and "True" or "False"), FieldMask, Property:GetByteMask())
        end
    else
        ValueStr = "UNHANDLED_VALUE"
    end

    return string.format("0x%04X    %s %s=%s", Property:GetOffset_Internal(), Property:GetClass():GetFName():ToString(), Property:GetFName():ToString(), ValueStr)
end

local function GetObjectByName(ObjectName)
    local Object = nil
    if FPackageName.IsShortPackageName(ObjectName) then
        Object = FindObject(nil, ObjectName, nil, nil)
    else
        Object = FindObject(nil, nil, ObjectName, false)
    end
    return Object
end

local function GetPropertyByName(ObjectIn, PropertyName)
    local PropertyFName = UEHelpers.FindOrAddFName(PropertyName)
    local Object = nil

    if ObjectIn:IsA(UClassStaticClass) or ObjectIn:IsA(UScriptStructStaticClass) then
        Object = ObjectIn
    else
        Object = ObjectIn:GetClass()
    end

    local PropertyFound = nil
    while Object:IsValid() do
        Object:ForEachProperty(function(Property)
            if Property:GetFName():Equals(PropertyFName) then
                PropertyFound = Property
                return true
            end
        end)
        Object = Object:GetSuperStruct()
    end

    return PropertyFound
end

DumpObject = function(Object)
    Log(string.format("*** Property dump for object '%s ***", Object:GetFullName()))

    -- Lets make sure that this is an object type that can be dumped.
    local IsClassCompatible = false
    if Object:IsA(UClassStaticClass) then IsClassCompatible = true end
    local IsUScriptStruct = Object:IsA(UScriptStructStaticClass)
    if IsUScriptStruct and not Object:IsMappedToObject() then IsClassCompatible = true end

    if IsClassCompatible then
        -- A UClass or UScriptStruct.
        local Class = Object
        while Class and Class:IsValid() do
            Log(string.format("=== %s properties ===", Class:GetFullName()))
            Class:ForEachProperty(function(Property)
                local OutputBuffer = string.format("0x%04X    %s %s", Property:GetOffset_Internal(), Property:GetClass():GetFName():ToString(), Property:GetFName():ToString())
                if Property:IsA(PropertyTypes.ObjectProperty) then
                    OutputBuffer = string.format("%s (%s)", OutputBuffer, Property:GetPropertyClass():GetFullName())
                elseif Property:IsA(PropertyTypes.BoolProperty) then
                    local FieldMask = Property:GetFieldMask()
                    if FieldMask ~= 255 then
                        OutputBuffer = string.format("%s (FM: 0x%X, BM: 0x%X)", OutputBuffer, FieldMask, Property:GetByteMask())
                    end
                end
                Log(OutputBuffer)
            end)

            Class = Class:GetSuperStruct()
        end
    elseif Object:IsA(UEnumStaticClass) then
        Object:ForEachName(function(Name, Value)
            Log(string.format("%s (%i)", Name:ToString(), Value))
        end)
    elseif not Object:IsA(UPropertyStaticClass) then
        -- A UObject that isn't a UClass, UScriptStruct, or UProperty (<4.25 only)
        local ObjectClass = nil
        if IsUScriptStruct then
            ObjectClass = Object
        else
            ObjectClass = Object:GetClass()
        end
        while ObjectClass and ObjectClass:IsValid() do
            Log(string.format("=== %s ===", ObjectClass:GetFullName()))
            ObjectClass:ForEachProperty(function(Property)
                Log(DumpPropertyWithinObject(Object, Property))
            end)

            ObjectClass = ObjectClass:GetSuperStruct()
        end
    end
end

local State = {
    ["StoreObjectContext"] = 1,
    ["FindProperty"] = 2,
    ["FoundObject"] = 3,
    ["FoundProperty"] = 4,
}

local ObjectContext = nil
local PropertyContext = nil
local CurrentState = State.StoreObjectContext

function ExecStateMachine(Param)
    if CurrentState == State.StoreObjectContext then
        if not Param or Param == "" or Param == " " then
            Log("No object name supplied")
            return true
        else
            ObjectContext = GetObjectByName(Param)
            if (ObjectContext:IsA(UScriptStructStaticClass)) or (ObjectContext and ObjectContext:IsValid()) then
                CurrentState = State.FindProperty
            else
                Log(string.format("No class found with name %s", Param))
                return true
            end
        end
    elseif CurrentState == State.FindProperty then
        PropertyContext = GetPropertyByName(ObjectContext, Param)
        if PropertyContext and PropertyContext:IsValid() then
            if PropertyContext:IsA(PropertyTypes.ObjectProperty) then
                --CurrentState = State.FindProperty
                --return ExecStateMachine(Param)
            end
            CurrentState = State.FoundProperty
        else
            Log(string.format("Property '%s' not found in '%s'", Param, ObjectContext:GetFullName()))
            return true
        end
    elseif CurrentState == State.FoundProperty then
        if PropertyContext:IsA(PropertyTypes.ArrayProperty) then
            PropertyContext = PropertyContext:GetInner()
            ObjectContext = ObjectContext[PropertyContext:GetFName():ToString()]

            local ArrayIndex = tonumber(Param)
            if not ArrayIndex then
                Log(string.format("Value '%s' is not a valid array index", Param))
                return true
            elseif ObjectContext:GetArrayNum() < ArrayIndex + 1 then
                Log(string.format("Array index '%i' out of bounds.", ArrayIndex))
                return true
            else
                ObjectContext = ObjectContext[ArrayIndex + 1]
                CurrentState = State.FindProperty
                return false
            end
        end

        ObjectContext = ObjectContext[PropertyContext:GetFName():ToString()]
        CurrentState = State.FindProperty
        return ExecStateMachine(Param)
    end
end

RegisterConsoleCommandHandler("dump_object", function(FullCommand, Parameters, Ar)
    ObjectContext = nil
    PropertyContext = nil
    CurrentState = State.StoreObjectContext

    if #Parameters < 1 then return false end
    local ShouldSaveToFile = Parameters[1]:find(".txt")
    if ShouldSaveToFile and #Parameters < 2 then return false end

    GlobalAr = Ar

    for i = ShouldSaveToFile and 2 or 1, #Parameters, 1 do
        local Param = Parameters[i]
        if ExecStateMachine(Param) then
            Buffer = ""
            return true
        end
    end

    if CurrentState == State.FoundProperty then
        Log(DumpPropertyWithinObject(ObjectContext, PropertyContext, true))
    else
        DumpObject(ObjectContext)
    end

    if ShouldSaveToFile then
        local File = io.open(Parameters[1], "w+")
        File:write(Buffer)
        File:close()

    end
    Buffer = ""
    return true
end)

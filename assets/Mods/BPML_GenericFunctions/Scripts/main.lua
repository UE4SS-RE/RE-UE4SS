--[[
    This is a mod where all BP functions available to BP mods goes.
    Only functions that don't need any information from BPModLoader goes in here.
]]

local VerboseLogging = true

local function Log(Message, AlwaysLog)
    if not VerboseLogging and not AlwaysLog then return end
    print(Message)
end

-- Explodes a string by a delimiter into a table
local function Explode(String, Delimiter)
    local ExplodedString = {}
    local Iterator = 1
    local DelimiterFrom, DelimiterTo = string.find(String, Delimiter, Iterator)

    while DelimiterTo do
        table.insert(ExplodedString, string.sub(String, Iterator, DelimiterFrom-1))
        Iterator = DelimiterTo + 1
        DelimiterFrom, DelimiterTo = string.find(String, Delimiter, Iterator)
    end
    table.insert(ExplodedString, string.sub(String, Iterator))

    return ExplodedString
end

RegisterCustomEvent("PrintToModLoader", function(ParamContext, ParamMessage)
    -- Retrieve the param value from the param container.
    local Message = ParamMessage:get()

    -- We must do type-checking here!
    -- This is to guard against mods that don't use the correct params for their custom event.
    -- There's no way to avoid it.
    if Message:type() ~= "FString" then error(string.format("PrintToModLoader Param #1 must be FString but was %s", Message:type())) end

    -- Now the 'Message' param is validated and we're safe to use it.
    local NameParts = Explode(ParamContext:get():GetClass():GetFullName(), "/");
    local ModName = NameParts[#NameParts - 1]
    Log(string.format("[%s] %s\n", ModName, Message:ToString()))
end)

local UClassStaticRef = nil
RegisterCustomEvent("ConstructPersistentObject", function(ParamContext, ParamClass, OutParam)
    -- Param Type Checking
    local Class = ParamClass:get()
    if not Class:IsValid() then error("ConstructPersistentObject Param #1 must be a valid UClass") end
    if not UClassStaticRef then
        UClassStaticRef = StaticFindObject("/Script/CoreUObject.Class")
    end
    if not UClassStaticRef:IsValid() then error("ConstructPersistentObject could not find /Script/CoreUObject.Class") end
    if not Class:IsA(UClassStaticRef) then error("ConstructPersistentObject Param #1 must be a UClass or UClass derivative") end
    local OutValue = OutParam:get()
    if not OutValue:type() == "UObject" then error("ConstructPersistentObject Return Value must be a UObject") end

    -- Function Logic
    local GameInstance = FindFirstOf("GameInstance")
    local GarbageCollectionKeepFlags = 0x0E000000
    local PersistentObject = StaticConstructObject(Class, GameInstance, 0, 0, GarbageCollectionKeepFlags, false, false, nil, nil, nil)
    if not PersistentObject:IsValid() then
        Log(string.format("Was unable to construct persistent object: %s", Class:GetFullName()))
    end

    -- Return Value
    OutParam:set(PersistentObject)
end)

local RegisteredBPHooks = {}
---@param Context UObject
---@param HookName FString Full name of the function you want to register a hook for e.g /Script/Engine.PlayerController:ClientRestart
---@param FunctionToCall FString Name of the function you want to be called from your own actor whenever the hooked function fires
--- NOTE: Don't forget to unregister your hooks whenever your Actor gets destroyed.
RegisterCustomEvent("RegisterHookFromBP", function (Context, HookName, FunctionToCall)
    -- Type checking similar to PrintToModLoader
    if HookName:get():type() ~= "FString" then error(string.format("RegisterHookFromBP Param #1 must be FString but was %s", HookName:get():type())) end
    if FunctionToCall:get():type() ~= "FString" then error(string.format("RegisterHookFromBP Param #2 must be FString but was %s", FunctionToCall:get():type())) end

    local HookName_AsString = HookName:get():ToString()

    -- Safeguard for checking if the function hook target is actually valid
    local OriginalFunction = StaticFindObject(HookName_AsString)
    if OriginalFunction == nil or not OriginalFunction:IsValid() or OriginalFunction:type() ~= "UFunction" then
        error(string.format("Tried to hook invalid function '%s'", HookName_AsString))
    end

    local FunctionName = FunctionToCall:get():ToString()
    local CallbackFunction = Context:get()[FunctionName]
    if CallbackFunction == nil or not CallbackFunction:IsValid() or CallbackFunction:type() ~= "UFunction" then
        error(string.format("Function '%s' doesn't exist in '%s'", FunctionName, Context:get():GetFullName()))
    end

    if RegisteredBPHooks[HookName_AsString] == nil then
        RegisteredBPHooks[HookName_AsString] = {}
    end

    local ExpectedContextClass = OriginalFunction:GetOuter()

    if not ExpectedContextClass:IsValid() or not ExpectedContextClass:IsClass() then
        error("Context Class was invalid or not a Class")
    end

    local OriginalTypes = {}
    local CallbackTypes = {}

    OriginalFunction:ForEachProperty(function(Property)
        if Property:HasAnyPropertyFlags(
            EPropertyFlags.CPF_ConstParm | 
            EPropertyFlags.CPF_Parm | 
            EPropertyFlags.CPF_OutParm | 
            EPropertyFlags.CPF_ReturnParm
        ) then
            table.insert(OriginalTypes, Property)
        end
    end)

    CallbackFunction:ForEachProperty(function(Property)
        if Property:HasAnyPropertyFlags(
            EPropertyFlags.CPF_ConstParm | 
            EPropertyFlags.CPF_Parm | 
            EPropertyFlags.CPF_OutParm | 
            EPropertyFlags.CPF_ReturnParm
        ) then
            table.insert(CallbackTypes, Property)
        end
    end)

    if #OriginalTypes+1 ~= #CallbackTypes then
        error("Param count did not match!")
    end

    if CallbackTypes[1]:GetPropertyClass():GetFullName() ~= ExpectedContextClass:GetFullName() then
        error(string.format("Context Class did not match the expected Class '%s'", ExpectedContextClass:GetFName():ToString()))
    end

    for i = 1, #OriginalTypes, 1 do
        -- Using i+1 here to skip Context from our Callback function, maybe there's a cleaner/better way?
        if ((CallbackTypes[i+1]:IsA(OriginalTypes[i]:GetClass())) or (type(OriginalTypes[i].IsFloatingPoint) == "function" and CallbackTypes[i+1]:IsFloatingPoint() and OriginalTypes[i]:IsFloatingPoint())) then
            
        else
            error("Param #" .. i .. " did not match the expected type '" .. OriginalTypes[i]:GetClass():GetFName():ToString() .. 
            "' got '" .. CallbackTypes[i+1]:GetClass():GetFName():ToString() .. "'")
        end
    end

    local ClassFullName = Context:get():GetClass():GetFullName()
    -- Prevent multiple hook registrations for a single function from the same Actor
    if RegisteredBPHooks[HookName_AsString][ClassFullName] ~= nil then
        UnregisterHook(
            HookName_AsString,
            RegisteredBPHooks[HookName_AsString][ClassFullName].PreHookId,
            RegisteredBPHooks[HookName_AsString][ClassFullName].PostHookId
        )
    end

    RegisteredBPHooks[HookName_AsString][ClassFullName] = {
        Owner = Context:get(),
        Function = CallbackFunction,
        PreHookId = 0,
        PostHookId = 0
    }

    local PreHookId, PostHookId = RegisterHook(HookName_AsString, function (Context, ...)
        for key, HookData in pairs(RegisteredBPHooks[HookName_AsString]) do
            ---@class UObject
            local Owner = HookData.Owner
            if Owner ~= nil and Owner:IsValid() then
                if HookData.Function ~= nil and HookData.Function:IsValid() then
                    local argTable = {}
                    local args = {...}
                    for i=1,#args do
                        table.insert(argTable, args[i]:get())
                    end

                    if #argTable > 0 then
                        HookData.Function(Owner, Context:get(), table.unpack(argTable, 1, #argTable))
                    else
                        HookData.Function(Owner, Context:get())
                    end
                end
            end
        end
    end)

    RegisteredBPHooks[HookName_AsString][ClassFullName].PreHookId = PreHookId
    RegisteredBPHooks[HookName_AsString][ClassFullName].PostHookId = PostHookId
end)

---@param HookName FString Full name of the function you want to unregister a hook for e.g /Script/Engine.PlayerController:ClientRestart
RegisterCustomEvent("UnregisterHookFromBP", function (Context, HookName)
    if HookName:get():type() ~= "FString" then error(string.format("UnregisterHookFromBP Param #1 must be FString but was %s", HookName:get():type())) end

    local HookName_AsString = HookName:get():ToString()

    if RegisteredBPHooks[HookName_AsString] == nil then
        return
    end

    local ClassFullName = Context:get():GetClass():GetFullName()

    if RegisteredBPHooks[HookName_AsString][ClassFullName] == nil then
        return
    end

    UnregisterHook(
        HookName_AsString,
        RegisteredBPHooks[HookName_AsString][ClassFullName].PreHookId,
        RegisteredBPHooks[HookName_AsString][ClassFullName].PostHookId
    )

    RegisteredBPHooks[HookName_AsString][ClassFullName] = nil
end)

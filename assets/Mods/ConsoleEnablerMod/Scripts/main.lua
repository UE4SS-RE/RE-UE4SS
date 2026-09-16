--########################
-- DEFINITIONS
--########################
-- state
local UEHelpers = require("UEHelpers")
local Pre, Post = -1, -1
local WasConsoleCreated = false
-- CONFIGURATION
-- you can edit the key names to your liking, make sure they match UE names
local IsDynamicViewport = false
local KeysToAdd = {
    UEHelpers.FindFName("Tilde"),
    UEHelpers.FindFName("F10")
}
local KeysToRemove = {
    -- Add keys here if you want to stop them from toggling the console.
    --UEHelpers.FindFName("@"),
}


--########################
-- OWN LOGIC
--########################
local function CopyArrayAsTable(A)
    local T = {}
    if type(A) ~= "userdata" then return T end
    if A:type() ~= "TArray" then return T end
    for i = 1, #A do
        -- Must actually re-create the struct, because it's owned by UE, and can be mutated by the owning array.
        -- For example, if TArray::Empty is called.
        T[i] = {KeyName = A[i].KeyName}
    end
    return T
end

local function TableContains(T, V)
    if type(T) ~= "table" then return false end
    for i = 1, #T do
        if T[i] == V then
            return true
        end
    end
    return false
end

local function RemapConsoleKeys()
    -- Change console key
    local InputSettings = StaticFindObject("/Script/Engine.Default__InputSettings") ---@cast InputSettings UInputSettings
    if not InputSettings:IsValid() then print("[ConsoleEnabler] InputSettings not found, could not change console key\n") return end

    local ConsoleKeys = InputSettings.ConsoleKeys
    for _, KeyName in ipairs(KeysToAdd) do
        if KeyName ~= NAME_None then
            local KeyIsAlreadySet = false
            for i = 1, #ConsoleKeys do
                if ConsoleKeys[i].KeyName == KeyName then
                    KeyIsAlreadySet = true
                    break
                end
            end
            if not KeyIsAlreadySet then
                ConsoleKeys[#ConsoleKeys + 1].KeyName = KeyName
            end
        end
    end

    if #KeysToRemove > 0 then
        -- Shenanigans required because the Lua API doesn't expose any way of removing elements from arrays.
        local ConsoleKeysCopy = CopyArrayAsTable(ConsoleKeys)
        ConsoleKeys:Empty()
        local NewConsoleArrayCount = 1
        for i = 1, #ConsoleKeysCopy do
            local Key = ConsoleKeysCopy[i]
            if not TableContains(KeysToRemove, Key.KeyName) then
                ConsoleKeys[NewConsoleArrayCount] = Key
                NewConsoleArrayCount = NewConsoleArrayCount + 1
            end
        end
    end

    for i = 1, #ConsoleKeys do
        print(string.format("[ConsoleEnabler] ConsoleKey[%d]: %s\n", i, ConsoleKeys[i].KeyName:ToString()))
    end
end

local function CreateConsole()
    local Engine = UEHelpers.GetEngine()
    if not Engine:IsValid() then print("[ConsoleEnabler] Was unable to find an instance of UEngine\n") return end

    local ConsoleClass = StaticFindObject("/Script/Engine.Console") ---@type UClass
    local GameViewport = Engine.GameViewport
    if (GameViewport:IsValid() and GameViewport.ViewportConsole:IsValid()) then
        -- Console already exists, let's just remap the keys
        WasConsoleCreated = true
        RemapConsoleKeys()
    elseif (ConsoleClass:IsValid() and GameViewport:IsValid()) then
        local CreatedConsole = StaticConstructObject(ConsoleClass, GameViewport) ---@cast CreatedConsole UConsole
        if not CreatedConsole:IsValid() then print("[CreateConsole] Was unable to construct an UConsole object\n") return end

        GameViewport.ViewportConsole = CreatedConsole
        WasConsoleCreated = true
        RemapConsoleKeys()
    else
        print("ConsoleClass, GameViewport, or ViewportConsole is invalid\n")
    end
end


--########################
-- ENTRY POINT
--########################

--- In cases where ClientRestart runs earlier than ExecuteInGameThread
if (not WasConsoleCreated or IsDynamicViewport) then
    ExecuteInGameThread(CreateConsole)
end

--- We only need to create console once since it is a VP singleton
Pre, Post = RegisterHook("/Script/Engine.PlayerController:ClientRestart",
---@param Context RemoteUnrealParam<APlayerController>
function(Context)
    if (not WasConsoleCreated or IsDynamicViewport) then
        CreateConsole()
    end
    if (WasConsoleCreated and not IsDynamicViewport) then
        UnregisterHook("/Script/Engine.PlayerController:ClientRestart", Pre, Post)
    end
end)

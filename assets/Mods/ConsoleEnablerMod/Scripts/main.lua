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


--########################
-- OWN LOGIC
--########################
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

    for i = 1, #ConsoleKeys do
        print(string.format("[ConsoleEnabler] ConsoleKey[%d]: %s\n", i, ConsoleKeys[i].KeyName:ToString()))
    end
end

local function CreateConsole()
    local Engine = UEHelpers.GetEngine()
    if not Engine:IsValid() then print("[ConsoleEnabler] Was unable to find an instance of UEngine\n") return end

    local ConsoleClass = Engine.ConsoleClass ---@type UClass
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

ExecuteInGameThread(CreateConsole)

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

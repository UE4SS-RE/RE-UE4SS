local UEHelpers = require("UEHelpers")

local PlayerControllerHookActive = false
local WasFirstConsoleCreated = false

local function RemapConsoleKeys()
    -- Change console key
    local InputSettings = StaticFindObject("/Script/Engine.Default__InputSettings") ---@cast InputSettings UInputSettings
    if not InputSettings:IsValid() then print("[ConsoleEnabler] InputSettings not found, could not change console key\n") return end

    local ConsoleKeys = InputSettings.ConsoleKeys

    local f10Name = UEHelpers.FindFName("F10")
    if f10Name == NAME_None then
        print("[RemapConsoleKeys] Was unable to find F10 FName\n")
        return
    end

    local f10AlreadySet = false
    for i = 1, #ConsoleKeys do
        local consoleKey = ConsoleKeys[i]
        local keyName = consoleKey.KeyName
        if keyName == f10Name then
            f10AlreadySet = true
        end
        print(string.format("[ConsoleEnabler] ConsoleKey[%d]: %s\n", i, keyName:ToString()))
    end
    if not f10AlreadySet then
        local newKeyIndex = #ConsoleKeys + 1
        ConsoleKeys[newKeyIndex].KeyName = f10Name
        print(string.format("[ConsoleEnabler] ConsoleKey[%d]: %s\n", newKeyIndex, f10Name:ToString()))
    end
end

local function CreateConsole()
    local Engine = UEHelpers.GetEngine()
    if not Engine:IsValid() then print("[ConsoleEnabler] Was unable to find an instance of UEngine\n") return end

    local ConsoleClass = Engine.ConsoleClass ---@type UClass
    local GameViewport = Engine.GameViewport

    if GameViewport:IsValid() and GameViewport.ViewportConsole:IsValid() then
        -- Console already exists, let's just remap the keys
        RemapConsoleKeys()
    elseif ConsoleClass:IsValid() and GameViewport:IsValid() then
        local CreatedConsole = StaticConstructObject(ConsoleClass, GameViewport) ---@cast CreatedConsole UConsole
        if not CreatedConsole:IsValid() then print("[CreateConsole] Was unable to construct an UConsole object\n") return end

        GameViewport.ViewportConsole = CreatedConsole
        PlayerControllerHookActive = true
        WasFirstConsoleCreated = true

        RemapConsoleKeys()
    else
        print("ConsoleClass, GameViewport, or ViewportConsole is invalid\n")
    end
end

CreateConsole()

NotifyOnNewObject("/Script/Engine.PlayerController", function()
    if PlayerControllerHookActive or not WasFirstConsoleCreated then
        CreateConsole()
    end
end)

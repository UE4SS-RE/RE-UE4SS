local UEHelpers = require("UEHelpers")

local PlayerControllerHookActive = false
local WasFirstConsoleCreated = false

local function RemapConsoleKeys()
    -- Change console key
    local InputSettings = StaticFindObject("/Script/Engine.Default__InputSettings") ---@cast InputSettings UInputSettings
    if not InputSettings:IsValid() then print("[ConsoleEnabler] InputSettings not found, could not change console key\n") return end

    local ConsoleKeys = InputSettings.ConsoleKeys

    local KeysToAdd = {
        UEHelpers.FindFName("Tilde"),
        UEHelpers.FindFName("F10")
    }

    for _, KeyName in ipairs(KeysToAdd) do
        if KeyName ~= NAME_None then
            local KeyIsAlreadySet = false
            for i = 1, #ConsoleKeys do
                if ConsoleKeys[i].KeyName == KeyName then
                    KeyIsAlreadySet = true
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

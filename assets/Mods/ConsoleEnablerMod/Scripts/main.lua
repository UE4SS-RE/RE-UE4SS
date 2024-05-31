local Engine = FindFirstOf("Engine")
local PlayerControllerHookActive = false
local WasFirstConsoleCreated = false

local function RemapConsoleKeys()
    -- Change console key
    local InputSettings = StaticFindObject("/Script/Engine.Default__InputSettings")
    if not InputSettings:IsValid() then print("[ConsoleEnabler] InputSettings not found, could not change console key\n") return end
    
    local ConsoleKeys = InputSettings.ConsoleKeys
    
    -- This sets the first console key to F10
    ConsoleKeys[1].KeyName = FName("F10", EFindName.FNAME_Find)
    
    ConsoleKeys:ForEach(function(index, elem_wrapper)
        local KeyStruct = elem_wrapper:get()
        local KeyFName = KeyStruct.KeyName
        -- The ToString() call here will go bad if the FName is not ansi
        print(string.format("[ConsoleEnabler] ConsoleKey[%d]: %s\n", index, KeyFName:ToString()))
    end)
end

local function CreateConsole()
    if not Engine:IsValid() then
        Engine = FindFirstOf("Engine")
    end
    
    if not Engine:IsValid() then print("[ConsoleEnabler] Was unable to find an instance of UEngine\n") return end
    
    local ConsoleClass = Engine.ConsoleClass
    local GameViewport = Engine.GameViewport
    
    if GameViewport.ViewportConsole:IsValid() then
        -- Console already exists, let's just remap the keys
        RemapConsoleKeys()
    elseif ConsoleClass:IsValid() and GameViewport:IsValid() then
        local CreatedConsole = StaticConstructObject(ConsoleClass, GameViewport, 0, 0, 0, nil, false, false, nil)
        GameViewport.ViewportConsole = CreatedConsole
        
        PlayerControllerHookActive = true
        WasFirstConsoleCreated = true

        RemapConsoleKeys()
    else
        print("ConsoleClass, GameViewport, or ViewportConsole is invalid\n")
    end
end

CreateConsole()

NotifyOnNewObject("/Script/Engine.PlayerController", function(CreatedObject)
    if PlayerControllerHookActive or not WasFirstConsoleCreated then
        CreateConsole()
    end
end)

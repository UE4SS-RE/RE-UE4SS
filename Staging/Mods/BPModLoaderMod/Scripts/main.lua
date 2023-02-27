local UEHelpers = require("UEHelpers")

local VerboseLogging = true

function Log(Message, AlwaysLog)
    if not VerboseLogging and not AlwaysLog then return end
    print(Message)
end

package.path = '.\\Mods\\ModLoaderMod\\?.lua;' .. package.path
package.path = '.\\Mods\\ModLoaderMod\\BPMods\\?.lua;' .. package.path

Mods = {}

local DefualtModConfig = {}
DefualtModConfig.AssetName = "ModActor_C"
DefualtModConfig.AssetNameAsFName = FName("ModActor_C")

-- Explodes a string by a delimiter into a table
function Explode(String, Delimiter)
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

function LoadModConfigs()
    -- Load configurations for mods.
    local Dirs = IterateGameDirectories();
    if not Dirs then
        error("[BPModLoader] UE4SS does not support loading mods for this game.")
    end
    local LogicModsDir = Dirs.Game.Content.Paks.LogicMods
    if not Dirs then error("[BPModLoader] IterateGameDirectories failed, cannot load BP mod configurations.") end
    if not LogicModsDir then
        CreateLogicModsDirectory();
        Dirs = IterateGameDirectories();
        LogicModsDir = Dirs.Game.Content.Paks.LogicMods
        if not LogicModsDir then error("[BPModLoader] Unable to find or create Content/Paks/LogicMods directory. Try creating manually.") end
    end
    for ModDirectoryName,ModDirectory in pairs(LogicModsDir) do
        Log(string.format("Mod: %s\n", ModDirectoryName))
        for _,ModFile in pairs(ModDirectory.__files) do
            Log(string.format("    ModFile: %s\n", ModFile.__name))
            if ModFile.__name == "config.lua" then
                dofile(ModFile.__absolute_path)
                if type(Mods[ModDirectoryName]) ~= "table" then break end
                if not Mods[ModDirectoryName].AssetName then break end
                Mods[ModDirectoryName].AssetNameAsFName = FName(Mods[ModDirectoryName].AssetName)
                break
            end
        end
    end

    -- Load a default configuration for mods that didn't have their own configuration.
    for _, ModFile in pairs(LogicModsDir.__files) do
        local ModName = ModFile.__name
        local ModNameNoExtension = ModName:match("(.+)%..+$")
        local FileExtension = ModName:match("^.+(%..+)$");
        if FileExtension == ".pak" and not Mods[ModNameNoExtension] then
            --Log("--------------\n")
            --Log(string.format("ModFile: '%s'\n", ModFile.__name))
            --Log(string.format("ModNameNoExtension: '%s'\n", ModNameNoExtension))
            --Log(string.format("FileExtension: %s\n", FileExtension))
            Mods[ModNameNoExtension] = {}
            Mods[ModNameNoExtension].AssetName = DefualtModConfig.AssetName
            Mods[ModNameNoExtension].AssetNameAsFName = DefualtModConfig.AssetNameAsFName
            Mods[ModNameNoExtension].AssetPath = string.format("/Game/Mods/%s/ModActor", ModNameNoExtension)
        end
    end
end

LoadModConfigs()

for k,v in pairs(Mods) do
    Log(string.format("%s == %s\n", k, v))
    if type(v) == "table" then
        for k2,v2 in pairs(v) do
            Log(string.format("    %s == %s\n", k2, v2))
        end
    end
end

local AssetRegistryHelpers = nil
local AssetRegistry = nil

function LoadMod(ModName, ModInfo, GameMode)
    Log(string.format("Loading mod: %s\n", ModName))
    if ModInfo.AssetPath == nil or ModInfo.AssetPath == nil then
        Log(string.format("Could not load mod '%s' because it has no asset path or name.\n", ModName))
    end

    local AssetData = nil
    if UnrealVersion.IsBelow(5, 1) then
        AssetData = {
            ["ObjectPath"] = FName(string.format("%s.%s", ModInfo.AssetPath, ModInfo.AssetName), EFindName.FNAME_Add),
        }
    else
        AssetData = {
            ["PackageName"] = FName(ModInfo.AssetPath, EFindName.FNAME_Add),
            ["AssetName"] = FName(ModInfo.AssetName, EFindName.FNAME_Add),
        }
    end

    ExecuteInGameThread(function()
        local ModClass = AssetRegistryHelpers:GetAsset(AssetData)
        if not ModClass:IsValid() then error("ModClass is not valid") end

        -- TODO: Use 'UEHelpers' to get the world.
        local World = GameMode:GetWorld()
        if not World:IsValid() then error("World is not valid") end

        local Actor = World:SpawnActor(ModClass, {}, {})
        if not Actor:IsValid() then
            Log(string.format("Actor for mod '%s' is not valid\n", ModName))
        else
            Log(string.format("Actor: %s\n", Actor:GetFullName()))
            local PreBeginPlay = Actor.PreBeginPlay
            if PreBeginPlay:IsValid() then
                Log(string.format("Executing 'PreBeginPlay' for mod '%s'\n", Actor:GetFullName()))
                PreBeginPlay()
            else
                Log("PreBeginPlay not valid\n")
            end
        end
    end)
end

function CacheAssetRegistry()
    if AssetRegistryHelpers and AssetRegistry then return end

    AssetRegistryHelpers = StaticFindObject("/Script/AssetRegistry.Default__AssetRegistryHelpers")
    if not AssetRegistryHelpers:IsValid() then print("AssetRegistryHelpers is not valid\n") end

    if AssetRegistryHelpers then
        AssetRegistry = AssetRegistryHelpers:GetAssetRegistry()
        if AssetRegistry:IsValid() then return end
    end

    AssetRegistry = StaticFindObject("/Script/AssetRegistry.Default__AssetRegistryImpl")
    if AssetRegistry:IsValid() then return end

    error("AssetRegistry is not valid\n")
end

function LoadMods(GameMode)
    CacheAssetRegistry()
    for ModName, ModInfo in pairs(Mods) do
        if type(ModInfo) == "table" then
            LoadMod(ModName, ModInfo, GameMode)
        end
    end
end

RegisterInitGameStatePostHook(function(ContextParam)
    LoadMods(ContextParam:get())
end)

RegisterBeginPlayPostHook(function(ContextParam)
    local Context = ContextParam:get()
    for _,ModConfig in pairs(Mods) do
        if Context:GetClass():GetFName() ~= ModConfig.AssetNameAsFName then return end
        local PostBeginPlay = Context.PostBeginPlay
        if PostBeginPlay:IsValid() then
            Log(string.format("Executing 'PostBeginPlay' for mod '%s'\n", Context:GetFullName()))
            PostBeginPlay()
        else
            Log("PostBeginPlay not valid\n")
        end
    end
end)

RegisterKeyBind(Key.INS, function()
    LoadMods(UEHelpers.GetWorld())
end)

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

RegisterCustomEvent("GetPersistentObject", function(ParamContext, ParamModName)
    -- TODO: Implement.
end)


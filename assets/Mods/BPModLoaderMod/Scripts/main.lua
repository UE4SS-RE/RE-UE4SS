---########################
--- DEFINES
---########################
package.path = '.\\Mods\\ModLoaderMod\\?.lua;' .. package.path
package.path = '.\\Mods\\ModLoaderMod\\BPMods\\?.lua;' .. package.path
local UEHelpers = require("UEHelpers")

local VerboseLogging = false
local SpawnModsOnLuaInit = true -- spawn on lua unit exec? common cause of random crashes (game-specific race?)
local AssetRegistryHelpers = CreateInvalidObject() ---@cast AssetRegistryHelpers UAssetRegistryHelpers
local AssetRegistry = CreateInvalidObject() ---@cast AssetRegistry IAssetRegistry

local Mods = {}
local OrderedMods = {}
local ModOrderList = {} -- contains entries from Mods/BPModLoaderMod/load_order.txt; used for the load order of BP mods.
local DefaultModConfig = {
    AssetName = "ModActor_C",
    AssetNameAsFName = UEHelpers.FindOrAddFName("ModActor_C")
}


---########################
--- HELPERS
---########################

local function Log(Message, OnlyLogIfVerbose)
    if not VerboseLogging and OnlyLogIfVerbose then return end
    print("[BPModLoaderMod] " .. Message)
end

--- get table count, never rely on #t
--- @param T table
--- @return integer
local function GetModCount()
    local c = 0
    for _, ModInfo in ipairs(OrderedMods) do
        if type(ModInfo) == "table" then c = c + 1 end
    end
    return c
end

-- Checks if the beginning of a string contains a certain pattern.
local function StartsWith(String, StringToCompare)
    return string.sub(String, 1, string.len(StringToCompare)) == StringToCompare
end

local function FileExists(file)
    local f = io.open(file, "rb")
    if f then f:close() end
    return f ~= nil
end

-- Reads lines from the specified file and returns a table of lines read.
-- Second argument takes a string that can be used to exclude lines starting with that string. Default ;
local function LinesFrom(file, ignoreLinesStartingWith)
    if not FileExists(file) then return {} end

    if ignoreLinesStartingWith == nil then
        ignoreLinesStartingWith = ";"
    end

    local lines = {}
    for line in io.lines(file) do
        if not StartsWith(line, ignoreLinesStartingWith) then
            lines[#lines + 1] = line
        end
    end
    return lines
end

local function LogOrderedMods()
    for _, v in ipairs(OrderedMods) do
        Log(string.format("%s == %s\n", v.Name, v))
        if type(v) == "table" then
            for k2, v2 in pairs(v) do
                Log(string.format("    %s == %s\n", k2, v2))
            end
        end
    end
end

-- Loads mod order data from load_order.txt and pushes it into ModOrderList.
local function LoadModOrder()
    local file = 'Mods/BPModLoaderMod/load_order.txt'
    local lines = LinesFrom(file)

    local entriesAdded = 0

    for _, line in pairs(lines) do
        ModAlreadyExists = false
        for _, ModName in pairs(ModOrderList) do
            if ModName == line then
                ModAlreadyExists = true
            end
        end
        -- Checks for double mod entries in the file and if a mod was already included, skip it.
        if not ModAlreadyExists then
            table.insert(ModOrderList, line)
            entriesAdded = entriesAdded + 1
        end
    end

    if entriesAdded <= 0 then
        Log(string.format("Mods/BPModLoaderMod/load_order.txt not present or no matching mods, loading all BP mods in random order.\n"))
    end
end

local function SetupModOrder()
    local Priority = 1

    -- Adds priority mods first by their respective order as specified in load_order.txt
    for _, ModOrderEntry in pairs(ModOrderList) do
        for ModName, ModInfo in pairs(Mods) do
            if type(ModInfo) == "table" then
                if ModOrderEntry == ModName then
                    OrderedMods[Priority] = ModInfo
                    OrderedMods[Priority].Name = ModName
                    OrderedMods[Priority].Priority = Priority
                    Priority = Priority + 1
                end
            end
        end
    end

    -- Adds the remaining mods in a random order after the prioritized mods.
    for ModName, ModInfo in pairs(Mods) do
        ModAlreadyIncluded = false
        for _, OrderedModInfo in ipairs(OrderedMods) do
            if type(OrderedModInfo) == "table" then
                if OrderedModInfo.Name == ModName then
                    ModAlreadyIncluded = true
                end
            end
        end

        if not ModAlreadyIncluded then
            ModInfo.Name = ModName
            table.insert(OrderedMods, ModInfo)
        end
    end
end

local function CacheAssetRegistry()
    ---@cast AssetRegistryHelpers UObject
    if (AssetRegistryHelpers:IsValid() and AssetRegistry:IsValid()) then return end

    AssetRegistryHelpers = StaticFindObject("/Script/AssetRegistry.Default__AssetRegistryHelpers") --[[@as UObject]]
    if (AssetRegistryHelpers ~= nil and AssetRegistryHelpers:IsValid()) then
        ---@cast AssetRegistryHelpers UAssetRegistryHelpers
        AssetRegistry = AssetRegistryHelpers:GetAssetRegistry() --[[@as IAssetRegistry]]
    end
    if not AssetRegistry:IsValid() then
        print("Failed to fetch AssetRegistry via ARHelpers, falling back to SFO search\n")
        AssetRegistry = StaticFindObject("/Script/AssetRegistry.Default__AssetRegistryImpl") --[[@as IAssetRegistry]]
    end
    if not AssetRegistry:IsValid() then error("Unable to continue - failed to validate UE game provides instance of AssetRegistry!\n") end
end

---########################
--- MAIN LOGIC
---########################
local function LoadModConfigs()
    -- Load configurations for mods.
    local Dirs = IterateGameDirectories();
    if not Dirs or not Dirs.Game.Content.Paks then
        error("[BPModLoader] UE4SS does not support loading mods for this game.")
    end
    local LogicModsDir = Dirs.Game.Content.Paks.LogicMods
    if not LogicModsDir then
        CreateLogicModsDirectory();
        Dirs = IterateGameDirectories(); ---@cast Dirs -nil
        LogicModsDir = Dirs.Game.Content.Paks.LogicMods
        if not LogicModsDir then
            error("[BPModLoader] Unable to find or create Content/Paks/LogicMods directory. Try creating manually.")
        end
        return
    end

    for ModDirectoryName, ModDirectory in pairs(LogicModsDir) do
        Log(string.format("Mod: %s\n", ModDirectoryName))
        for _, ModFile in pairs(ModDirectory.__files) do
            Log(string.format("    ModFile: %s\n", ModFile.__name))
            if ModFile.__name == "config.lua" then
                dofile(ModFile.__absolute_path)
                if type(Mods[ModDirectoryName]) ~= "table" then break end
                if not Mods[ModDirectoryName].AssetName then break end
                Mods[ModDirectoryName].AssetNameAsFName = UEHelpers.FindOrAddFName(Mods[ModDirectoryName].AssetName)
                break
            end
        end
    end

    local Files = LogicModsDir.__files
    for _, ModDirectory in pairs(LogicModsDir) do
        for _, ModFile in pairs(ModDirectory.__files) do
            table.insert(Files, ModFile)
        end
    end

    -- Load a default configuration for mods that didn't have their own configuration.
    for _, ModFile in pairs(Files) do
        local ModName = ModFile.__name
        local ModNameNoExtension = ModName:match("(.+)%..+$")
        local FileExtension = ModName:match("^.+(%..+)$");
        if FileExtension == ".pak" and not Mods[ModNameNoExtension] then
            --Log("--------------\n")
            --Log(string.format("ModFile: '%s'\n", ModFile.__name))
            --Log(string.format("ModNameNoExtension: '%s'\n", ModNameNoExtension))
            --Log(string.format("FileExtension: %s\n", FileExtension))
            Mods[ModNameNoExtension] = {}
            Mods[ModNameNoExtension].AssetName = DefaultModConfig.AssetName
            Mods[ModNameNoExtension].AssetNameAsFName = DefaultModConfig.AssetNameAsFName
            Mods[ModNameNoExtension].AssetPath = string.format("/Game/Mods/%s/ModActor", ModNameNoExtension)
        end
    end

    LoadModOrder()
    SetupModOrder()
end

local function LoadMod(ModName, ModInfo, World)
    if ModInfo.Priority ~= nil then
        Log(string.format("Loading mod [Priority: #%i]: %s\n", ModInfo.Priority, ModName))
    else
        Log(string.format("Loading mod: %s\n", ModName))
    end

    if ModInfo.AssetPath == nil or ModInfo.AssetPath == nil then
        Log(string.format("Could not load mod '%s' because it has no asset path or name.\n", ModName))
        return
    end

    local AssetData = nil
    if UnrealVersion.IsBelow(5, 1) then
        AssetData = {
            ["ObjectPath"] = UEHelpers.FindOrAddFName(string.format("%s.%s", ModInfo.AssetPath, ModInfo.AssetName)),
        }
    else
        AssetData = {
            ["PackageName"] = UEHelpers.FindOrAddFName(ModInfo.AssetPath),
            ["AssetName"] = UEHelpers.FindOrAddFName(ModInfo.AssetName),
        }
    end
    ---@cast AssetRegistryHelpers UObject
    if (not AssetRegistryHelpers:IsValid()) then
        error("Unable to continue - AssetRegistryHelpers is invalid")
    end

    ---@cast AssetRegistryHelpers UAssetRegistryHelpers
    local ModClass = AssetRegistryHelpers:GetAsset(AssetData)
    if not ModClass:IsValid() then
        local ObjectPath = AssetData.ObjectPath and AssetData.ObjectPath:ToString() or ""
        local PackageName = AssetData.PackageName and AssetData.PackageName:ToString() or ""
        local AssetName = AssetData.AssetName and AssetData.AssetName:ToString() or ""
        Log(string.format("ModClass for '%s' is not valid\nObjectPath: %s\nPackageName: %s\nAssetName: %s\n", ModName, ObjectPath,PackageName, AssetName))
        return
    end

    if not World then error("A `nil` World parameter was passed to LoadMod function. It's most likely a bug in BPModLoaderMod!") end
    if not World:IsValid() then
        Log(string.format("World is not valid for '%s' to spawn in\n", ModName))
        return
    end

    local Actor = World:SpawnActor(ModClass, {}, {})
    if not Actor:IsValid() then
        Log(string.format("Actor for mod '%s' is not valid\n", ModName))
    else
        Log(string.format("Actor: %s\n", Actor:GetFullName()))
        local PreBeginPlay = Actor.PreBeginPlay
        if PreBeginPlay:IsValid() then
            Log(string.format("Executing 'PreBeginPlay' for mod '%s', with path: '%s'\n", ModName, Actor:GetFullName()))
            PreBeginPlay()
        else
            Log(string.format("PreBeginPlay not valid for mod %s\n", ModName), true)
        end
    end
end

local function LoadMods(World)
    if not World or not World:IsValid() then
        Log("[Warning] Invalid UWorld object was passed to LoadMods.\n")
        return
    end

    CacheAssetRegistry()
    for _, ModInfo in ipairs(OrderedMods) do
        if type(ModInfo) == "table" then
            LoadMod(ModInfo.Name, ModInfo, World)
        end
    end
end


---########################
--- ENTRY POINT
---########################

LoadModConfigs()
LogOrderedMods()

--- Only execute/add any hooks if we have at least one mod entry
--- It does not matter we can do it manually on a hotkey, PAKs arent mounted
--- automatially at runtime on LUA reload and we dont need potentially unstable and moot hooks
if (GetModCount() > 0) then

    RegisterKeyBind(Key.INS, function()
        ExecuteInGameThread(function()
            LoadMods(UEHelpers.GetWorld())
        end)
    end)

    RegisterBeginPlayPostHook(function(ContextParam)
        local Context = ContextParam:get()
        for _, ModConfig in ipairs(OrderedMods) do
            if Context:GetClass():GetFName() ~= ModConfig.AssetNameAsFName then return end
            local AssetPathWithClassPrefix = string.format("BlueprintGeneratedClass %s.%s", ModConfig.AssetPath, ModConfig.AssetName)
            if AssetPathWithClassPrefix == Context:GetClass():GetFullName() then
                local PostBeginPlay = Context.PostBeginPlay
                if PostBeginPlay:IsValid() then
                    Log(string.format("Executing 'PostBeginPlay' for mod '%s'\n", Context:GetFullName()))
                    PostBeginPlay()
                else
                    Log(string.format("PostBeginPlay not valid for mod %s\n", Context:GetFullName()), true)
                end
            end
        end
    end)

    RegisterLoadMapPostHook(function(Engine, World)
        LoadMods(World:get())
    end)

    if (SpawnModsOnLuaInit) then
        ExecuteInGameThread(function()
            local ExistingActor = FindFirstOf("Actor")
            if ExistingActor:IsValid() then
                LoadMods(ExistingActor:GetWorld())
            end
        end)
    end
else
    Log(string.format("No PAK mod entries found, skipping hooking the events for this run\n"), false)
end

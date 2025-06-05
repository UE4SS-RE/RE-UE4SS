---@param self RemoteUnrealParam<APlayerController>
---@param NewPawn RemoteUnrealParam<APawn>
RegisterHook("/Script/Engine.PlayerController:ClientRestart", function(self, NewPawn)
    local PlayerController = self:get()

    if PlayerController.CheatManager:IsValid() then
        print("[CheatManagerEnabler] CheatManager already exist, skipping restoration\n")
        return
    end

    local CheatManagerClass = PlayerController.CheatClass
    if not CheatManagerClass:IsValid() then
        print("[CheatManagerEnabler] Controller:CheatClass is nullptr, using default CheatClass instead\n")

        CheatManagerClass = StaticFindObject("/Script/Engine.CheatManager") --[[@as UClass]]
    end

    if not CheatManagerClass:IsValid() then
        print("[CheatManagerEnabler] Couldn't find default CheatClass, therefore, could not enable Cheat Manager\n")
        return
    end

    local CreatedCheatManager = StaticConstructObject(CheatManagerClass, PlayerController)
    if CreatedCheatManager:IsValid() then
        print(string.format("[CheatManagerEnabler] Constructed CheatManager [0x%X]\n", CreatedCheatManager:GetAddress()))

        PlayerController.CheatManager = CreatedCheatManager
        print("[CheatManagerEnabler] Enabled CheatManager\n")
    else
        print("[CheatManagerEnabler] Was unable to construct CheatManager, therefore, could not enable Cheat Manager\n")
    end
end)

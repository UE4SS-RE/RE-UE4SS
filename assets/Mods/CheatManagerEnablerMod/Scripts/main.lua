RegisterHook("/Script/Engine.PlayerController:ClientRestart", function(self, NewPawn)
    local PlayerController = self:get()
    
    local CheatManagerClass = PlayerController.CheatClass
    if not CheatManagerClass:IsValid() then
        print("[CheatManager Creator] Controller:CheatClass is nullptr, using default CheatClass instead\n")
        
        CheatManagerClass = StaticFindObject("/Script/Engine.CheatManager")
    end
    
    if not CheatManagerClass:IsValid() then
        print("[CheatManager Creator] Couldn't find default CheatClass, therefore, could not enable Cheat Manager\n")
        return
    end
    
    local CreatedCheatManager = StaticConstructObject(CheatManagerClass, PlayerController, 0, 0, 0, nil, false, false, nil)
    if CreatedCheatManager:IsValid() then
        print(string.format("[CheatManager Creator] Constructed CheatManager [0x%X]\n", CreatedCheatManager:GetAddress()))
        
        PlayerController.CheatManager = CreatedCheatManager
        print("[CheatManager Creator] Enabled CheatManager\n")
    else
        print("[CheatManager Creator] Was unable to construct CheatManager, therefor, could not enable Cheat Manager\n")
    end
end)

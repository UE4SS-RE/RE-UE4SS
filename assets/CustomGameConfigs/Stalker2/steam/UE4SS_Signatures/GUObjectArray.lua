-- Stalker2-Win64-Shipping.exe
-- SHA-256: D4BC8617CB79250B37AC8DB97972ABB52C784167806A565F93C01970CA3A2594

function Register()
    return "39 3D ? ? ? ? 0F 8E ? ? ? ? 0F B7 C7 48 8B 0D ? ? ? ? 89 FA C1 EA 10 4C 8D 34 40 41 C1 E6 03 4C 03 34 D1 C1 E5 17 4D 8B 26 4D 85 E4 0F 85 ? ? ? ? 09 DD"
end

function OnMatchFound(MatchAddress)
    local MovInstruction = MatchAddress + 0xF
    local Displacement = DerefToInt32(MovInstruction + 0x3)
    if Displacement == nil then
        return 0
    end

    -- The MOV addresses FUObjectArray.ObjObjects, which starts at +0x10.
    return MovInstruction + 0x7 + Displacement - 0x10
end
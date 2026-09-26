-- Stalker2-Win64-Shipping.exe
-- SHA-256: D4BC8617CB79250B37AC8DB97972ABB52C784167806A565F93C01970CA3A2594

function Register()
    return "56 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 31 E0 48 89 84 24 ? ? ? ? E8 ? ? ? ? 48 89 05 ? ? ? ?"
end

function OnMatchFound(MatchAddress)
    local MovInstruction = MatchAddress + 0x20
    local Displacement = DerefToInt32(MovInstruction + 0x3)
    if Displacement == nil then
        return 0
    end

    return MovInstruction + 0x7 + Displacement
end
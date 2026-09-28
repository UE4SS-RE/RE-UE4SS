-- Stalker2-WinGDK-Shipping.exe
-- SHA-256: 82F0BA79013F163732A6E7D1F1BEC50515F18B52F640BA57E0261BD8EBFE0E1F

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

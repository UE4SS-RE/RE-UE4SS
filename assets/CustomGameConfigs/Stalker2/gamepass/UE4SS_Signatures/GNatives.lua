-- Stalker2-WinGDK-Shipping.exe
-- SHA-256: 82F0BA79013F163732A6E7D1F1BEC50515F18B52F640BA57E0261BD8EBFE0E1F

function Register()
    return "41 56 56 57 53 48 83 EC ? 0F 29 74 24 ? 48 89 D6 48 8B 05 ? ? ? ? 48 31 E0 48 89 44 24 ? 0F 57 F6 0F 11 72 30 48 C7 42 40 00 00 00 00 48 8B 4A 18 48 8B 42 20 48 8D 50 01 48 89 56 20 0F B6 00 4C 8D 35 ? ? ? ? 48 89 F2 45 31 C0 41 FF 14 C6 48 8B 7E 30"
end

function OnMatchFound(MatchAddress)
    local LeaInstruction = MatchAddress + 0x42
    local Displacement = DerefToInt32(LeaInstruction + 0x3)
    if Displacement == nil then
        return 0
    end

    return LeaInstruction + 0x7 + Displacement
end

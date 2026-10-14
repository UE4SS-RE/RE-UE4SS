-- Stalker2-WinGDK-Shipping.exe
-- SHA-256: 82F0BA79013F163732A6E7D1F1BEC50515F18B52F640BA57E0261BD8EBFE0E1F

function Register()
    return "48 83 EC ? 8B 05 ? ? ? ? 8B 0D ? ? ? ? 65 48 8B 14 25 58 00 00 00 48 8B 0C CA 3B 81 E0 00 00 00 7F ? 48 8D 05 ? ? ? ? 48 83 C4 ? C3 48 8D 0D ? ? ? ? E8 ? ? ? ? 83 3D ? ? ? ? FF 75 ? E8 ? ? ? ? 49 B8 00 00 00 00 00 00 00 10"
end

function OnMatchFound(MatchAddress)
    return MatchAddress
end

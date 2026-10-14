-- StaticConstructObject_Internal beginning bytes:
-- 4C 8B DC 55 53 41 56 49 8D AB 38 FE FF FF 48 81 EC B0 02 00 00 48 8B ?? ?? ?? ?? ?? 48 33 C4 48 89 85 90 01 00 00 8B 41 70 33 DB 49 89 73 10 49 89 7B 18 48 8B F9 4D 89 63 20 44 8B 61 18 4D 89 6B E0 4C 8B 69 08 4D 89 7B D8 4C 8B 39 89 44 24 68 41 F7 87

function Register()
    return "E8 ?? ?? ?? ?? 48 83 7C 24 70 00 48 8B F8 74 1D 48 8B 94 24 98 00 00 00 48 8D 8C 24 80 00 00 00 48 85 D2 48 0F 45 CA 48 8B 11 FF 52 10 80 3D ?? ?? ?? ?? ?? 74 0D 48 85 FF 74 08 48 8B CF E8 ?? ?? ?? ?? BA"
end

function OnMatchFound(MatchAddress)
    local CallInstr = MatchAddress
    local InstrSize = 5
    local NextInstr = CallInstr + InstrSize
    local Offset = DerefToInt32(CallInstr + 1)
    local Address = NextInstr + Offset
    return Address
end
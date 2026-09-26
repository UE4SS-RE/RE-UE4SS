function Register()
    -- Two options for indirect scans:
    -- 1: E8 ?? ?? ?? ?? 48 8B 54 24 58 48 85 D2 74 18 33 C9 45 33 C9 41 B8 04 00 00 00 E8
    --    E8 ?? ?? ?? ?? 48 8B 54 24 58 48 85 D2 74 18

    -- 2: E8 ?? ?? ?? ?? 48 8B 95 88 00 00 00 48 89 7D 38 48 3B D7 75 12
    --    E8 ?? ?? ?? ?? 48 8B 95 88 00 00 00 48 89 7D 38 48 3B D7
    --    E8 ?? ?? ?? ?? 48 8B 95 88 00 00 00 48 89 7D 38
    return "E8 ?? ?? ?? ?? 48 8B 54 24 58 48 85 D2 74 18"
end

function OnMatchFound(MatchAddress)
    local CallInstr = MatchAddress
    local InstrSize = 5
    local NextInstr = CallInstr + InstrSize
    local Offset = DerefToInt32(CallInstr + 1)
    local Address = NextInstr + Offset
    return Address
end
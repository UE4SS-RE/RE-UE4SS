function Register()
    return "E8 ?? ?? ?? ?? 4C 8B 6C 24 58 4D 89 2E"
end

function OnMatchFound(MatchAddress)
    local CallInstr = MatchAddress
    local InstrSize = 5
    local NextInstr = CallInstr + InstrSize
    local Offset = DerefToInt32(CallInstr + 1)
    local FNameConstructorAddress = NextInstr + Offset
    return FNameConstructorAddress
end
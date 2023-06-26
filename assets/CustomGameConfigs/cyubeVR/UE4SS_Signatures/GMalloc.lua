function Register()
    return "48 8B ?? ?? ?? ?? ?? 48 8B 01 FF 50 78 8B ?? ?? ?? ?? ?? 8B"
end

function OnMatchFound(MatchAddress)
    local MovInstr = MatchAddress
    local NextInstr = MovInstr + 0x7
    local Offset = MovInstr + 0x3
    local AddressMoved = NextInstr + DerefToInt32(Offset)
    return AddressMoved
end
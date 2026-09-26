function Register()
    return "C7 44 24 20 00 00 00 00 48 8D 0D ? ? ? ? 48 89 F2 41 B9 FF FF FF FF E8"
end

function OnMatchFound(MatchAddress)
    local LeaInstr = MatchAddress + 8
    local NextInstr = LeaInstr + 0x7
    local Offset = LeaInstr + 0x3
    local GUObjectArrayAddress = NextInstr + DerefToInt32(Offset)
    return GUObjectArrayAddress
end

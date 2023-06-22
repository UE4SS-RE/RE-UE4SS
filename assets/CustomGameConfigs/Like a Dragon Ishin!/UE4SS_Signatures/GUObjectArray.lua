function Register()
    return "48 8D ?? ?? ?? ?? ?? 44 8B 84 24 90 00 00 00 8B 94 24 98 00 00 00 E8 ?? ?? ?? ?? E8"
end

function OnMatchFound(MatchAddress)
    local LeaInstr = MatchAddress
    local NextInstr = LeaInstr + 0x7
    local Offset = LeaInstr + 0x3
    local AddressLoaded = NextInstr + DerefToInt32(Offset)
    return AddressLoaded
end
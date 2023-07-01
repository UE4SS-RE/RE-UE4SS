function Register()
    return "48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? C6 05 ?? ?? ?? ?? ?? 80 3D"
end

function OnMatchFound(MatchAddress)
    local LeaInstr = MatchAddress
    local NextInstr = LeaInstr + 0x7
    local Offset = LeaInstr + 0x3
    local AddressLoaded = NextInstr + DerefToInt32(Offset)
    return AddressLoaded
end
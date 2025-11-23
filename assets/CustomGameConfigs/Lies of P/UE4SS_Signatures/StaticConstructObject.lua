function Register()
    return "48 89 ?? 24 30 89 ?? 24 38 E8 ?? ?? ?? ?? 48 ?? ?? 24 70 48 ?? ?? 24 78"
end

function OnMatchFound(MatchAddress)
    local NextInstr = MatchAddress + 14
    local Offset = MatchAddress + 10
    local AddressLoaded = NextInstr + DerefToInt32(Offset)
    return AddressLoaded
end
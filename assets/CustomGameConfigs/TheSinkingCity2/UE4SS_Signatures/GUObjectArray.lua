function Register()
    return "48 83 EC 28 48 8D 0D ?? ?? ?? ?? E8 00 91 74 00 48 8D 0D 59 F1 94 07 48 83 C4 28 E9 FC 5F 8C 07"
end

function OnMatchFound(MatchAddress)
    return MatchAddress + 0xB + DerefToInt32(MatchAddress + 0x7)
end
function Register()
    return "48 89 5C 24 10 48 89 74 24 18 48 89 7C 24 20 55 41 54 41 55 41 56 41 57 48 8D AC 24 F0 FD FF FF 48 81 EC 10 03 00 00 48 8B ?? ?? ?? ?? ?? 48 33 C4 48 89 85 00 02 00 00"
end

function OnMatchFound(MatchAddress)
    return MatchAddress
end

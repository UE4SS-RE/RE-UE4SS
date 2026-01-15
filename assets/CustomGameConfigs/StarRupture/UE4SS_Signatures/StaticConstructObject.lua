function Register()
    return "48 89 5C 24 10 48 89 74 24 18 48 89 7C 24 20 55 41 54 41 55 41 56 41 57 48 8D AC 24 10 FE FF FF 48 81 EC F0 02 00 00 48 8B ?? ?? ?? ?? ?? 48 33 C4 48 89 85 E0 01 00 00 48 8B 71 28"
end

function OnMatchFound(MatchAddress)
    return MatchAddress
end

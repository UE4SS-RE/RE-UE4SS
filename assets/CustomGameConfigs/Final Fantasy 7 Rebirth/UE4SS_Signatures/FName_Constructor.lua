function Register()
    return "48 89 5C 24 10 48 89 6C 24 18 56 57 41 56 B8 40 04 00 00 E8 ?? ?? ?? ?? 48 2B E0 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 84 24 30 04 00 00 45 33 F6"
end

function OnMatchFound(matchAddress)
    return matchAddress
end

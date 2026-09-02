function Register()
    return "48 89 5C 24 08 48 89 74 24 20 57 B8 70 00 00 00 E8 ?? ?? ?? ?? 48 2B E0 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 60 48 8B 42 20 49 8B F0 48 8B DA EB 15 48 8B 53 18 4C 8D 44 24 20 48 8B CB E8 ?? ?? ?? ?? 48 8B 43 20 80 38 04 75 E6"
end

function OnMatchFound(matchAddress)
    return matchAddress
end

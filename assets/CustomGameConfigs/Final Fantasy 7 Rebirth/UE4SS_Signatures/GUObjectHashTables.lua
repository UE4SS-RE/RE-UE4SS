function Register()
    return "E8 ?? ?? ?? ?? 33 F6 48 8B F8 33 C0 F0 0F B1 35"
end

function OnMatchFound(matchAddress)
    return matchAddress + 5 + DerefToInt32(matchAddress + 1)
end

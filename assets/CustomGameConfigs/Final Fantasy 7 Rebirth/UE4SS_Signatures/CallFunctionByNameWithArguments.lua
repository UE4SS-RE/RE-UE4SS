function Register()
    return "B8 38 00 00 00 E8 ?? ?? ?? ?? 48 2B E0 C6 44 24 20 00 E8 05 00 00 00"
end

function OnMatchFound(matchAddress)
    local callInstr = matchAddress + 18
    return callInstr + 5 + DerefToInt32(callInstr + 1)
end

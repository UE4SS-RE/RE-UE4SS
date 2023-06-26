function Register()
    return "48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 45 33 C9 4C 89 74 24 20 4C 8D 44 24 20 4C 89 7C 24 28 48 8D 54 24 60 48 8D 4E 08 E8 ?? ?? ?? ?? 48 85 DB"
end

function OnMatchFound(matchAddress)
    local leaInstr = matchAddress
    local nextInstr = leaInstr + 0x7
    local offset = leaInstr + 0x3
    local arrayAddress = nextInstr + DerefToInt32(offset)
    
    return arrayAddress
end
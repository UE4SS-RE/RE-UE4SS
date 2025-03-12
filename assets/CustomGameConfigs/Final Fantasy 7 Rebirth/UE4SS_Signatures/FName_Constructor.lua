function Register()
    return "41 B8 01 00 00 00 48 8D 4C 24 ?? E8 ?? ?? ?? ?? C6 44 24 48 00"
end

function OnMatchFound(matchAddress)
    local nextInstr = matchAddress + 16
    local offset = matchAddress + 12
    local dataMoved = nextInstr + DerefToInt32(offset)
    
    return dataMoved
end
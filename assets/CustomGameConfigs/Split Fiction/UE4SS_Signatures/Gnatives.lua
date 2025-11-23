function Register()
    return "48 8D 05 ?? ?? ?? ?? B9 1F 00 00 00 0F"
end

function OnMatchFound(matchAddress)
    local nextInstr = matchAddress + 7
    local offset = matchAddress + 3
    local dataMoved = nextInstr + DerefToInt32(offset)
    
    return dataMoved
end
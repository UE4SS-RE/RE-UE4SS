function Register()
    return "03 ?? ?? ?? ?? ?? FF C8 3B D0 0F 8D ?? ?? ?? ?? 44 8B"
end

function OnMatchFound(matchAddress)
    local nextInstr = matchAddress + 6
    local offset = matchAddress + 2
    local dataMoved = nextInstr + DerefToInt32(offset)
    
    return dataMoved
end
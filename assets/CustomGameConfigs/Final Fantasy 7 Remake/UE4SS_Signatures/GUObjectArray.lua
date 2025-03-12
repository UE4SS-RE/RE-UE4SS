function Register()
    return "48 8B ?? ?? ?? ?? ?? 4C 8B 04 C8 4D 85 C0 74 07 ?? ?? ?? 0F 94"
end

function OnMatchFound(matchAddress)
    local nextInstr = matchAddress + 0x7
    local offset = matchAddress + 0x3
    local dataMoved = nextInstr + DerefToInt32(offset)-0x10
    
    return dataMoved
end
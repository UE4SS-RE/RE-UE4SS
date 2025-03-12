function Register()
	return "E8 ?? ?? ?? ?? 48 8B 7D 78 4C 8B F0"
end

function OnMatchFound(matchAddress)
    local nextInstr = matchAddress + 5
    local offset = matchAddress + 1
    local dataMoved = nextInstr + DerefToInt32(offset)
    
    return dataMoved
end
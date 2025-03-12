function Register()
    return "E8 ?? ?? ?? ?? 45 33 C0 48 8D 55 ?? 49 8B CF 4C 8B E8 E8 ?? ?? ?? ??"
end

function OnMatchFound(matchAddress)
    local nextInstr = matchAddress + 5
    local offset = matchAddress + 1
	local dataMoved = nextInstr + DerefToInt32(offset)
	
    return dataMoved
end
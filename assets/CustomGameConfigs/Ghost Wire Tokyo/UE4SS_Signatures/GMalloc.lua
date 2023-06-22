function Register()
    --return "48 8B ?? ?? ?? ?? ?? 49 8B F1 49 8B E8 4C 8B F2 48 85 C9 74 50"
    return "4C 8B ?? ?? ?? ?? ?? 49 8B 00 48 8B D3 49 8B C8 FF 50 30 48 8B 5C 24 50 48 85 DB 74 3A"
end

function OnMatchFound(MatchAddress)
    local MovInstr = MatchAddress
    local NextInstr = MovInstr + 0x7
    local Offset = MovInstr + 0x3
    local DataMoved = NextInstr + DerefToInt32(Offset)
    
    return DataMoved
end
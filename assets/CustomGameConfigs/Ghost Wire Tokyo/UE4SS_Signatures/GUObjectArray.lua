function Register()
    return "48 8D 0D ? ? ? ? 49 83 62 10 00 41 89 42 08 45 84 C0 49 89 0A"
end

function OnMatchFound(MatchAddress)
    local LeaInstr = MatchAddress
    local NextInstr = LeaInstr + 7
    local Offset = LeaInstr + 3
    return NextInstr + DerefToInt32(Offset)
end

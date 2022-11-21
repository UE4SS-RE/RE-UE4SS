function Register()
    return "E 8/? ?/? ?/? ?/? ?/4 8/8 B/4 4/2 4/3 0/4 8/8 9/? ?/? ?/? ?/? ?/? ?/C 7/0 5"
end

function OnMatchFound(MatchAddress)
    local CallInstr = MatchAddress
    local InstrSize = 5
    local NextInstr = CallInstr + InstrSize
    local Offset = DerefToInt32(CallInstr + 1)
    local FNameConstructorAddress = NextInstr + Offset
    return FNameConstructorAddress
end
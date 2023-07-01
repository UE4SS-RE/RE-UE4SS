function Register()
    return "E 8/? ?/? ?/? ?/? ?/4 8/6 3/9 E/4 0/0 1/0 0/0 0"
end

function OnMatchFound(MatchAddress)
    local CallInstr = MatchAddress
    local InstrSize = 5
    local NextInstr = CallInstr + InstrSize
    local Offset = DerefToInt32(CallInstr + 1)
    local ToStringAddress = NextInstr + Offset
    return ToStringAddress
end
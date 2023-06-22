function Register()
    --return "E 8/? ?/? ?/? ?/? ?/4 8/8 B/4 C/2 4/? ?/8 B/F D/4 8/8 5/C 9"
    return "41 57 41 56 41 55 41 54 56 57 53 48 81 EC 20 08 00 00 49 89 D5 49 89 CF"
end

function OnMatchFound(MatchAddress)
    --local AOBSize = 22
    --local CallInstr = MatchAddress
    --local InstrSize = 5
    --local NextInstr = CallInstr + InstrSize
    --local Offset = DerefToInt32(CallInstr + 1)
    --local ToStringAddress = NextInstr + Offset
    --return ToStringAddress
    return MatchAddress
end
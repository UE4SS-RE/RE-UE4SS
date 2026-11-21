function Register()
    return "48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 30 E8 ?? ?? ?? ?? 8A 0D ?? ?? ?? ?? 48 8B F0 90 84 C9"
end

function OnMatchFound(MatchAddress)
    local CallInstruction = MatchAddress + 0xF
    return CallInstruction + 0x5 + DerefToInt32(CallInstruction + 0x1)
end

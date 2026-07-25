function Register()
    return "48 C7 ? 10 00 00 00 00 48 8D 05 ? ? ? ? ? ? ? 48 89 ? ? FF FF FF FF 89 ? 08"
end

function OnMatchFound(MatchAddress)
    local LeaInstruction = MatchAddress + 0x8
    local NextInstruction = LeaInstruction + 0x7
    local DisplacementAddress = LeaInstruction + 0x3
    return NextInstruction + DerefToInt32(DisplacementAddress)
end

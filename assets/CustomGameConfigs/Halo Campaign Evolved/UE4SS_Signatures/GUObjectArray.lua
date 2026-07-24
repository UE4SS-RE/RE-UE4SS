function Register()
    return "45 84 C0 48 C7 41 10 00 00 00 00 48 8D 05 ? ? ? ? 4C 8B C9 48 89 01 B8 FF FF FF FF 89 41 08"
end

function OnMatchFound(MatchAddress)
    local LeaInstruction = MatchAddress + 0xB
    local NextInstruction = LeaInstruction + 0x7
    local DisplacementAddress = LeaInstruction + 0x3
    return NextInstruction + DerefToInt32(DisplacementAddress)
end

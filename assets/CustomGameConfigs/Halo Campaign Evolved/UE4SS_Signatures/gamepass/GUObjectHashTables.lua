-- Halo: Campaign Evolved Game Pass

function Register()
    return "48 89 5C 24 08 57 48 83 EC 20 48 8B D9 E8 ?? ?? ?? ?? 48 8B C8 48 8D 53 F8 E8 ?? ?? ?? ?? 48 8B 4B 08 48 85 C9"
end

function OnMatchFound(MatchAddress)
    -- Decode the first E8 rel32 CALL at +0xD; its target is FUObjectHashTables::Get().
    local CallInstruction = MatchAddress + 0xD
    local NextInstruction = CallInstruction + 0x5
    local DisplacementAddress = CallInstruction + 0x1
    return NextInstruction + DerefToInt32(DisplacementAddress)
end

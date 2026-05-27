-- GNatives global variable for UE 5.7
-- Indirect scan: finds the GNatives initialization function, resolves RIP-relative address
function Register()
    return "48 8D 05 ?? ?? ?? ?? 48 39 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 74 ?? C7 05 ?? ?? ?? ?? 00 00 00 00 C3"
end

function OnMatchFound(MatchAddress)
    -- The "mov [GNatives], rax" instruction is at offset 0x15 from pattern start
    -- It's a 7-byte instruction: 48 89 05 [4-byte RIP offset]
    -- GNatives address = next_instruction + offset
    local movInstr = MatchAddress + 0x15
    local nextInstr = movInstr + 7
    local offset = DerefToInt32(movInstr + 3)
    return nextInstr + offset
end

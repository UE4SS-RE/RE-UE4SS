-- Phantom Blade Zero (UE 5.7, PBZ.exe FileVersion CL-301524_JKS-17219, Steam build 26266538).
-- The pattern and OnMatchFound are unchanged from the UE4SS team's custom game config for Far Far West
-- (zCustomGameConfigs.zip of UE4SS v3.0.1-1183-gcdbc0b66, MIT; its note says UE 5.8). In PBZ.exe the pattern matches
-- exactly once (RVA 0xEB5830); the result lies in the exe's writable data. Without this file UE4SS 1183 logs
-- "Failed to find GNatives" and its scan never ends.
-- GNatives global variable for UE 5.8
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

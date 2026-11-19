-- STAR WARS Galactic Racer (UE 5.7, SWGR-Win64-Shipping.exe FileVersion Auto-386500, Steam build 26133805).
-- Matches an inlined FFrame::Step: the opcode byte is read from Stack.Code, then
-- "call [GNatives + opcode*8]" runs with Result = nullptr.
-- The pattern matches twice; both resolve to the same address.
function Register()
    return "0F B6 00 4C 8D 0D ?? ?? ?? ?? 48 89 FA 45 31 C0 41 FF 14 C1"
end

function OnMatchFound(MatchAddress)
    -- LEA R9, [RIP+disp32] (GNatives) is a seven-byte instruction at +0x3.
    local LeaInstr = MatchAddress + 0x3
    local NextInstr = LeaInstr + 0x7
    return NextInstr + DerefToInt32(LeaInstr + 0x3)
end

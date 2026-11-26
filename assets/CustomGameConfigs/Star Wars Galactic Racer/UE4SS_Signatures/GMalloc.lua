-- STAR WARS Galactic Racer (UE 5.7, SWGR-Win64-Shipping.exe FileVersion Auto-386500, Steam build 26133805).
-- Matches FMemory::Free (clang-cl build): after the null check on the pointer it loads GMalloc into RSI,
-- null checks it, then compares vtable slot 0x48 (Free) for speculative devirtualization.
-- The pattern matches twice (FMemory::Free and a duplicate); both resolve to the same address.
function Register()
    return "48 85 C9 0F 84 ?? ?? ?? ?? 48 89 CF 48 8B 35 ?? ?? ?? ?? 48 85 F6 0F 84 ?? ?? ?? ?? 48 8B 06 48 8B 40 48"
end

function OnMatchFound(MatchAddress)
    -- MOV RSI, [RIP+disp32] (GMalloc) is a seven-byte instruction at +0xC.
    local MovInstr = MatchAddress + 0xC
    local NextInstr = MovInstr + 0x7
    return NextInstr + DerefToInt32(MovInstr + 0x3)
end

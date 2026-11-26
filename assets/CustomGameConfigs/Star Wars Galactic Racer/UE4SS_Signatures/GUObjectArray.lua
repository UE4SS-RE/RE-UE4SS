-- STAR WARS Galactic Racer (UE 5.7, SWGR-Win64-Shipping.exe FileVersion Auto-386500, Steam build 26133805).
-- The game is built with clang-cl, so the MSVC-shaped GUObjectArray patterns in patternsleuth don't match.
-- This matches the Fatal log in FUObjectArray::AllocateUObjectIndex ("Unable to add more objects to disregard
-- for GC pool (Max: %d)"), which reads ObjLastNonGCIndex (+0x4) and MaxObjectsNotConsideredByGC (+0x8).
-- AllocateUObjectIndex is inlined into two callers, so the pattern matches twice; both resolve to the same address.
function Register()
    return "48 8D 0D ?? ?? ?? ?? 4C 8D 05 ?? ?? ?? ?? BA 0A 01 00 00 E8 ?? ?? ?? ?? 8B 05 ?? ?? ?? ?? 44 8B 0D ?? ?? ?? ??"
end

function OnMatchFound(MatchAddress)
    -- MOV R9D, [RIP+disp32] (MaxObjectsNotConsideredByGC) is a seven-byte instruction at +0x1E.
    local MovInstr = MatchAddress + 0x1E
    local NextInstr = MovInstr + 0x7
    local MaxObjectsNotConsideredByGC = NextInstr + DerefToInt32(MovInstr + 0x3)
    return MaxObjectsNotConsideredByGC - 0x8
end

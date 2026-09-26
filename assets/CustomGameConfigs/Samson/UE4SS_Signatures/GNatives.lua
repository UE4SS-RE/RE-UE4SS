-- GNatives for Samson (CJSteam-Win64-Shipping.exe, UE 5.7).
-- Anchor: an inlined FFrame::Step inside P_GET_* exec thunks:
--   lea r9, [rip + GNatives] ; mov rcx, [rbx + 0x18] (Stack.Object)
-- 2415 matches in .text, every one resolves to the same GNatives, so the first match is enough.
-- Found/verified by patches/Samson/find_optional_globals.py.
function Register()
    return "4C 8D 0D ?? ?? ?? ?? 48 8B 4B 18"
end

function OnMatchFound(MatchAddress)
    -- lea r9, [rip + disp32]: disp at +3, instruction length 7
    return MatchAddress + 7 + DerefToInt32(MatchAddress + 3)
end

-- FName::FName(wchar_t const *, enum EFindName) for Far Far West
-- Updated for Steam buildid 24978658 (Aug 2026 update).
function Register()
    return "40 53 48 83 EC 30 48 8B D9 48 89 54 24 20 33 C9 4C 8B CA 44 8B C1 48 85 D2 74 ?? 0F B7 02 66 85 C0"
end

function OnMatchFound(MatchAddress)
    print("[FName_Constructor] OnMatchFound called with: " .. string.format("0x%X", MatchAddress))
    -- Match address IS the function start; return unchanged.
    return MatchAddress
end

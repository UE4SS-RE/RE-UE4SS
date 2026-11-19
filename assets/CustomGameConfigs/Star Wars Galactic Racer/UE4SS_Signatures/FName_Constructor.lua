-- STAR WARS Galactic Racer (UE 5.7, SWGR-Win64-Shipping.exe FileVersion Auto-386500, Steam build 26133805).
-- FName::FName(const WIDECHAR*, EFindName), clang-cl build. Most call sites inline it; this is the out-of-line copy.
-- It null checks the string, scans it for non-ASCII chars, then calls FNameHelper::ParseNumber.
function Register()
    return "41 56 56 57 53 48 81 EC 48 04 00 00 44 89 C3 48 89 CE 48 8B 05 ?? ?? ?? ?? 48 31 E0 48 89 84 24 40 04 00 00 48 89 54 24 20 31 FF 48 85 D2 0F 84 ?? ?? ?? ?? 44 0F B7 02"
end

function OnMatchFound(MatchAddress)
    return MatchAddress
end

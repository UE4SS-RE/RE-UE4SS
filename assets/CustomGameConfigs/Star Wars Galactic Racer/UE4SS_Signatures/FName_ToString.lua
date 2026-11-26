-- STAR WARS Galactic Racer (UE 5.7, SWGR-Win64-Shipping.exe FileVersion Auto-386500, Steam build 26133805).
-- void FName::ToString(FString&) const, clang-cl build. Without this file patternsleuth finds two unrelated
-- candidates and rejects both. The pattern runs through the FNamePool block lookup and the
-- "cmp dword ptr [rdi+4], 0" Number check, which tells it apart from FName::AppendString (same prologue).
function Register()
    return "41 57 41 56 41 55 41 54 56 57 55 53 48 81 EC 38 08 00 00 48 89 D6 48 89 CF 48 8B 05 ?? ?? ?? ?? 48 31 E0 48 89 84 24 30 08 00 00 8B 19 80 3D ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? 89 D8 C1 E8 10 48 8D 0D ?? ?? ?? ?? 48 8B 44 C1 10 0F B7 CB 48 8D 1C 48 0F B7 03 41 89 C0 41 C1 E8 06 83 7F 04 00"
end

function OnMatchFound(MatchAddress)
    return MatchAddress
end

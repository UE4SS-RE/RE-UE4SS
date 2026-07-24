-- Halo Campaign Evolved ProcessLocalScriptFunction (RVA 0x394E7C0).
-- RIP-relative displacements are wildcarded for relocation-safe matching.

function Register()
    return "48 89 5C 24 20 55 56 57 41 54 41 57 48 83 EC 70 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 60 48 8B 42 20 4C 8D 7A 18 48 8B 6A 10 4C 8D 25 ?? ?? ?? ??"
end

function OnMatchFound(MatchAddress)
    return MatchAddress
end
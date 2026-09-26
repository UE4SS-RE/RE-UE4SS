-- SWZeroCompany.exe
-- SHA-256: D12F2318D1B9DA731A631340E147B226C5844B62110662F3DAAA36AD91FE2571
-- public: ?ProcessLocalScriptFunction@@YAXPEAVUObject@@AEAUFFrame@@QEAX@Z

function Register()
    return "4C 8B DC 53 55 41 54 41 56 41 57 48 83 EC ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 48 8B 42 20 4C 8D 72 18 4C 8B 7A 10"
end

function OnMatchFound(MatchAddress)
    return MatchAddress
end
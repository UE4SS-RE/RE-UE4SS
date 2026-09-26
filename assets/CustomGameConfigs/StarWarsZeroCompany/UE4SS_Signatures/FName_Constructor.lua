-- SWZeroCompany.exe
-- SHA-256: D12F2318D1B9DA731A631340E147B226C5844B62110662F3DAAA36AD91FE2571
-- public: FName::FName(wchar_t const*, EFindName)

function Register()
    return "48 89 5C 24 ? 57 48 83 EC ? 4C 8B CA 48 89 54 24 ?"
end

function OnMatchFound(MatchAddress)
    return MatchAddress
end
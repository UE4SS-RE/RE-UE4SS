-- SWZeroCompany.exe
-- SHA-256: C69131D496756EA421E408261FBA33B60613948E2C480ACAC91CB93632A4B67C
-- UObject* StaticConstructObject_Internal(const FStaticConstructObjectParameters&)

function Register()
    return "4C 8B DC 55 53 41 56 49 8D AB ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 8B 41 70"
end

function OnMatchFound(MatchAddress)
    return MatchAddress
end
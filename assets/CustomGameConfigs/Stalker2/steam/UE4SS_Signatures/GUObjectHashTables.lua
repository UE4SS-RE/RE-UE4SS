-- Stalker2-Win64-Shipping.exe (Steam, 2026-09-19 build)
-- FUObjectHashTables::Get() is inlined everywhere in this build, so this returns the singleton
-- itself. UE4SS (UEPseudo 36e87ab+) treats a non-executable address as the object and validates it
-- with the hash-table self test before use.
-- Anchor: ShrinkUObjectHashTables console command, which inlines Get():
--   test byte [rip+X], 1 ; jne ; lea rsi, [rip+Singleton] ; mov rcx, rsi ; call [EnterCriticalSection]
-- Unique in .text. Found/verified by Vibe-Reverse-Engineering/patches/Stalker2/verify_signature.py.
local LEA_OFFSET = 9 -- lea rsi, [rip+disp32]: 48 8D 35 <disp32>

function Register()
    return "F6 05 ?? ?? ?? ?? 01 75 ?? 48 8D 35 ?? ?? ?? ?? 48 89 F1"
end

function OnMatchFound(MatchAddress)
    local Lea = MatchAddress + LEA_OFFSET
    return Lea + 7 + DerefToInt32(Lea + 3)
end

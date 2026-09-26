-- FUObjectHashTables::Get() signature for Ghostwire Tokyo (GWT.exe, UE 4.27)
--
-- FUObjectHashTables::Get() is a thread-safe magic-static accessor. UE4SS calls
-- the resolved function to obtain the FUObjectHashTables singleton (located at
-- 0x145DD4D18 in this build).
--
--   sub  rsp, 0x28
--   mov  rax, gs:[0x58]                 ; TLS array
--   mov  edx, 0xF8
--   mov  rcx, [rax]
--   mov  eax, [rdx+rcx]                 ; per-thread init epoch
--   cmp  [rip+guard_epoch], eax         ; 39 05 <disp32>
--   jg   init
--   lea  rax, [rip+FUObjectHashTables]  ; 48 8D 05 <disp32>  -> &singleton
--   add  rsp, 0x28
--   ret
--
-- The magic-static prologue is shared by hundreds of getters in the binary, so
-- the guard/lea rip-displacements are kept fixed to uniquely identify this one.
-- (rip-displacements are relative offsets, so this is still ASLR-independent;
-- it only needs regenerating if the game executable is patched/recompiled.)

function Register()
    return "48 83 EC 28 65 48 8B 04 25 58 00 00 00 BA F8 00 00 00 48 8B 08 8B 04 0A 39 05 BE 13 45 05 7F 0C 48 8D 05 BD 13 45 05 48 83 C4 28 C3"
end

function OnMatchFound(MatchAddress)
    return MatchAddress
end

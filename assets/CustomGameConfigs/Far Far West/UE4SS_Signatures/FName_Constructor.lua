-- FName::FName(wchar_t const *, enum EFindName) for UE 5.7 (Far Far West launch)
-- Direct scan starting at the function entry. Single wildcard added on the
-- "08" stack-offset byte to make UE4SS's Lua scanner actually find the
-- pattern (the no-wildcard prologue version is not picked up by the scanner
-- for unknown reasons, even though x64dbg confirms the bytes are present).
--
-- Verified function start: 0x7FF6099C81A0 (after int3 padding).
function Register()
    -- Function prologue with one wildcard at byte 4:
    --   mov [rsp+??], rbx ; push rdi ; sub rsp, 0x30 ; mov rbx, rcx ;
    --   mov [rsp+0x20], rdx ; xor ecx, ecx ; mov edi, r8d ; mov r10, rdx ;
    --   mov r9d, ecx ; test rdx, rdx
    return "48 89 5C 24 ?? 57 48 83 EC 30 48 8B D9 48 89 54 24 20 33 C9 41 8B F8 4C 8B D2 44 8B C9 48 85 D2"
end

function OnMatchFound(MatchAddress)
    print("[FName_Constructor] OnMatchFound called with: " .. string.format("0x%X", MatchAddress))
    -- Match address IS the function start; return unchanged.
    return MatchAddress
end

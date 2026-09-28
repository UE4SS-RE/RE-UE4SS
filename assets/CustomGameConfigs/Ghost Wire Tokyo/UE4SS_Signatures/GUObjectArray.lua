-- GUObjectArray signature for Ghostwire Tokyo (GWT.exe, UE 4.27)
--
-- Anchored on the FUObjectArray::TIterator constructor, which stores the address
-- of GUObjectArray into the iterator object:
--   lea  rcx, [GUObjectArray]      ; 48 8D 0D <disp32>
--   and  qword [r10+0x10], 0       ; 49 83 62 10 00
--   mov  [r10+8], eax              ; 41 89 42 08
--   test r8b, r8b                  ; 45 84 C0
--   mov  [r10], rcx                ; 49 89 0A   (stores &GUObjectArray)
--
-- The lea's rip-relative displacement is wildcarded and resolved at runtime, so
-- the pattern is ASLR-independent. Resolves to 0x145D2BC90 in this build.

function Register()
    return "48 8D 0D ? ? ? ? 49 83 62 10 00 41 89 42 08 45 84 C0 49 89 0A"
end

function OnMatchFound(MatchAddress)
    local LeaInstr = MatchAddress
    local NextInstr = LeaInstr + 7
    local Offset = LeaInstr + 3
    return NextInstr + DerefToInt32(Offset)
end

-- ProcessLocalScriptFunction signature for Ghostwire Tokyo (GWT.exe, UE 4.27)
--
-- UE4SS auto-derives this by disassembling ProcessInternal and following a call;
-- on this customized 4.27 build that heuristic picks the WRONG call and lands on
-- __security_check_cookie (0x142475000). UE4SS then installs its script hook on
-- the cookie-check routine, which runs at the epilogue of nearly every function,
-- causing widespread memory corruption (the FName-pool / vtable-dispatch crashes).
--
-- The real ProcessLocalScriptFunction is the UE bytecode interpreter: it reads the
-- FFrame.Code pointer ([rcx+0x20]), fetches the opcode, and dispatches through
-- GNatives:
--   mov   rax, [rcx+0x20]        ; FFrame.Code
--   movzx r9d, byte [rax]        ; opcode
--   lea   r10, [rax+1]           ; Code++
--   mov   [rcx+0x20], r10
--   lea   r9, [rip+GNatives]     ; 4C 8D 0D <disp32>  -> 0x145CB5BE0
--   mov   r9, [r9 + rax*8]       ; GNatives[opcode]
--   jmp   r9
--
-- Resolves to 0x140ADD0BC in this build. The GNatives-lea displacement is
-- wildcarded so the pattern is ASLR-independent.

function Register()
    return "48 8B 41 20 4C 8B DA 44 0F B6 08 4C 8D 50 01 41 8B C1 4C 89 51 20 4C 8D 0D ? ? ? ?"
end

function OnMatchFound(MatchAddress)
    return MatchAddress
end

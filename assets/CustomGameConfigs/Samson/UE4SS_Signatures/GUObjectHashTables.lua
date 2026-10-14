-- FUObjectHashTables::Get() for Samson (CJSteam-Win64-Shipping.exe, UE 5.7).
-- Identified as the first call in ShrinkUObjectHashTables (console command); the result is
-- locked with FHashTableLock. Get() is a thread-safe function-local static (94 callers).
-- Anchor: `lea rcx, [rip+X]; xor eax, eax; xorps xmm0, xmm0` in the singleton's first-use
-- initialisation, unique in .text, 0x61 bytes into the function. Step back and confirm the
-- first 8 bytes (push rbx; sub rsp,0x20; mov rax,gs:[0x58]) so a stale match fails safely.
-- Found/verified by patches/Samson/find_optional_globals.py and find_minimal_aob.py.
local OFFSET_TO_FUNCTION_START = 0x61
local PROLOGUE_INT32_0 = -2092412096 -- 0x83485340 as signed int32: 40 53 48 83
local PROLOGUE_INT32_4 = 0x486520EC  -- EC 20 65 48

function Register()
    return "48 8D 0D ?? ?? ?? ?? 33 C0 0F 57 C0"
end

function OnMatchFound(MatchAddress)
    local FunctionStart = MatchAddress - OFFSET_TO_FUNCTION_START
    if DerefToInt32(FunctionStart) ~= PROLOGUE_INT32_0 or DerefToInt32(FunctionStart + 4) ~= PROLOGUE_INT32_4 then
        print("[GUObjectHashTables.lua] prologue mismatch, signature needs updating\n")
        return nil
    end
    return FunctionStart
end

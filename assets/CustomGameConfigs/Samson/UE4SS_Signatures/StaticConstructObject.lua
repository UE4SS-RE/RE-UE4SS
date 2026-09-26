-- StaticConstructObject_Internal for Samson (CJSteam-Win64-Shipping.exe, UE 5.7).
-- Anchor: `test dword [Class + ClassFlags], CLASS_Native | CLASS_Intrinsic (0x10000080)`,
-- unique in .text. It sits 0x52 bytes into the function, so step back and confirm the
-- prologue (`mov r11, rsp; push rbp`) before returning; a mismatch fails the scan safely.
-- Found/verified by patches/Samson/find_static_construct_object.py.
local OFFSET_TO_FUNCTION_START = 0x52
local PROLOGUE_INT32 = 0x55DC8B4C -- 4C 8B DC 55

function Register()
    return "F7 87 ?? ?? ?? ?? 80 00 00 10"
end

function OnMatchFound(MatchAddress)
    local FunctionStart = MatchAddress - OFFSET_TO_FUNCTION_START
    if DerefToInt32(FunctionStart) ~= PROLOGUE_INT32 then
        print("[StaticConstructObject.lua] prologue mismatch, signature needs updating\n")
        return nil
    end
    return FunctionStart
end

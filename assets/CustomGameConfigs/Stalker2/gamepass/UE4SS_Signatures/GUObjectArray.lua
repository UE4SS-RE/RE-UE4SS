-- Stalker2-WinGDK-Shipping.exe
-- SHA-256: 82F0BA79013F163732A6E7D1F1BEC50515F18B52F640BA57E0261BD8EBFE0E1F

function Register()
    return "39 05 ? ? ? ? 7E ? 0F B7 C8 48 8B 15 ? ? ? ? C1 E8 10 48 8D 0C 49 C1 E1 03 48 03 0C C2 EB ? 45 31 F6 EB ? 31 C9 4C 8B 31"
end

function OnMatchFound(MatchAddress)
    local MovInstruction = MatchAddress + 0xB
    local Displacement = DerefToInt32(MovInstruction + 0x3)
    if Displacement == nil then
        return 0
    end

    -- The MOV addresses FUObjectArray.ObjObjects, which starts at +0x10.
    return MovInstruction + 0x7 + Displacement - 0x10
end

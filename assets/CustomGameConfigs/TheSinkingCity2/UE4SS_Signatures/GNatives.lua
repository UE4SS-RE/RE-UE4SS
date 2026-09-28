--[[
-- Alternative signature.

function Register()
    return "48 8D ?? ?? ?? ?? ?? 0F 1F 40 00 0F 1F 84 00 00 00 00 00 48 8B 13"
end

function OnMatchFound(MatchAddress)
    local LeaInstr = MatchAddress
    local NextInstr = LeaInstr + 0x7
    local Offset = LeaInstr + 0x3
    local AddressLoaded = NextInstr + DerefToInt32(Offset)
    return AddressLoaded
end
]]

function Register()
    return "4C 89 63 38 4C 89 63 40 8B 46 38 48 0F BA E0 08 73 05 4D 8B C4 EB 07 4C 63 46 44 4C 03 C7"
end

function OnMatchFound(MatchAddress)
    local Address = MatchAddress - 0x15
    return Address + 0x7 + DerefToInt32(Address + 0x3)
end

function Register()
  return "48 8D 0D ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? C6 05 ? ? ? ? 01"
end

function OnMatchFound(MatchAddress)
  local LeaInstr = MatchAddress
  local NextInstr = LeaInstr + 0x7
  local Offset = LeaInstr + 0x3
  local AddressLoaded = NextInstr + DerefToInt32(Offset) - 0x10
  return AddressLoaded
end

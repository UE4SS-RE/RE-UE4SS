-- Phantom Blade Zero (UE 5.7, PBZ.exe FileVersion CL-301524_JKS-17219, Steam build 26266538).
-- The pattern and OnMatchFound are unchanged from the UE4SS team's custom game configs for Metal Gear Solid Delta and
-- Split Fiction (zCustomGameConfigs.zip of UE4SS v3.0.1-1183-gcdbc0b66, MIT). In PBZ.exe the pattern matches exactly
-- once (RVA 0x166C147, next to StaticConstructObject_Internal and FUObjectHashTables::Get); the result lies in the
-- exe's writable data. Without this file UE4SS 1183 logs "Failed to find GUObjectArray" and its scan never ends.
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

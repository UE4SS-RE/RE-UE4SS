function Register()
	return
	"40 53 48 83 EC 30 48 8B D9 48 89 54 24 20 33 C9 4C 8B CA 44 8B C1 48 85 D2 74 ?? 0F B7 02 66 85 C0 74 ?? 0F 1F 40 00 66 0F 1F 84 00 00 00 00 00"
end

function OnMatchFound(MatchAddress)
	return MatchAddress
end

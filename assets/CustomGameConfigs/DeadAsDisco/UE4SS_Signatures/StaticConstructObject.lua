function Register()
    return "4C 8B 47 10 48 8D 44 24 64 48 89 44 24"
end

function OnMatchFound(MatchAddress)
    local v1 = 0xD3
    return MatchAddress - v1
end

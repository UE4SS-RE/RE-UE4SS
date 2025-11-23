function Register()
  return "48 89 5C 24 08 57 48 83 EC 30 45 33 DB 48"
end

function OnMatchFound(matchAddress)
  print("Match found at address: " .. matchAddress)
  return matchAddress
end

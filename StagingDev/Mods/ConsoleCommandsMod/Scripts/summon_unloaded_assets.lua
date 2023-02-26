RegisterConsoleCommandHandler("summon", function(FullCommand, Parameters)
    -- If we have no parameters then just let someone else handle this command
    if #Parameters < 1 then return false end
    
    LoadAsset(Parameters[1])
    
    -- Return true if this is the final handler for this command
    -- Return false if you want another handler to be able to handle this command
    -- In this case, we want the CheatManager to also handle this command
    return false
end)
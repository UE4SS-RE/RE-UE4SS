RegisterCustomProperty({
    ["Name"] = "Actors",
    ["Type"] = PropertyTypes.ArrayProperty,
    ["BelongsToClass"] = "/Script/Engine.Level",
    ["OffsetInternal"] = 0x98,
    ["ArrayProperty"] = {
        ["Type"] = PropertyTypes.ObjectProperty
    }
})

RegisterKeyBind(Key.NUM_THREE, {ModifierKey.CONTROL}, function()
    local Level = FindFirstOf("Level")
    local Actors = Level.Actors
    
    Actors:ForEach(function(Index, ElemWrapper)
        local Actor = ElemWrapper:get()
        
        print(string.format("0x%X %s\n", Actor:GetAddress(), Actor:GetFullName()))
    end)
end)

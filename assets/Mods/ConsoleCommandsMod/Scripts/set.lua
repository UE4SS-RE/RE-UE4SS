RegisterConsoleCommandHandler("set", function(FullCommand, Parameters, Ar)
    -- If we have no parameters then just let someone else handle this command
    if #Parameters < 3 then return false end
    
    GlobalAr = Ar
    
    local ClassOrObjectName = Parameters[1]
    local PropertyName = Parameters[2]
    local NewPropertyValue = Parameters[3]
    
    local BannedFlags = EObjectFlags.RF_ClassDefaultObject | EObjectFlags.RF_ArchetypeObject
    
    if not ClassOrObjectName or ClassOrObjectName == "" or ClassOrObjectName == " " then
        Log("No class or object name supplied")
        return true
    end
    
    local ObjectOrClass = FindObject(nil, ClassOrObjectName, nil, BannedFlags)
    if not ObjectOrClass:IsValid() then
        Log(string.format("Unrecognized class or object %s", ClassOrObjectName))
        return true
    end
    
    -- Replace with a call to "IsA" when it exists
    if (ObjectOrClass:IsClass()) then
        local Property = ObjectOrClass:Reflection():GetProperty(PropertyName)
        if Property:IsValid() then
            local Objects = FindObjects(0, ObjectOrClass, nil, nil, BannedFlags, false)
            for ObjectNum, Object in pairs(Objects) do
                Property:ImportText(NewPropertyValue, Property:ContainerPtrToValuePtr(Object), 0, Object)
            end
            
            if #Objects == 0 then
                Log(string.format("No objects found with class %s", ClassOrObjectName))
            end
        else
            Log(string.format("Unrecognized property %s on class %s", PropertyName, ClassOrObjectName))
        end
    else
        local Property = ObjectOrClass:Reflection():GetProperty(PropertyName)
        if Property:IsValid() then
            Property:ImportText(NewPropertyValue, Property:ContainerPtrToValuePtr(ObjectOrClass), 0, ObjectOrClass)
        else
            Log(string.format("Unrecognized property %s on class %s", PropertyName, ClassOrObjectName))
        end
    end
    
    return true
end)

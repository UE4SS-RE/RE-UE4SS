local has_run_probe = false
local dumpers_scheduled = false

local function marker(message)
    print(message .. "\n")
end

local function fail(step, detail)
    marker(string.format("ACCEPT:FAIL step=%s detail=%s", step, tostring(detail)))
end

local function schedule_dumpers()
    if dumpers_scheduled or os.getenv("UE4SS_ACCEPTANCE_RUN_DUMPERS") ~= "1" then
        return
    end
    dumpers_scheduled = true

    local delay_ms = tonumber(os.getenv("UE4SS_ACCEPTANCE_DUMPER_DELAY_MS") or "30000") or 30000
    marker(string.format("ACCEPT:LUA_DUMPERS_SCHEDULED delay_ms=%d", delay_ms))
    ExecuteWithDelay(delay_ms, function()
        ExecuteInGameThread(function()
            marker("ACCEPT:LUA_DUMP_OBJECTS_REQUESTED")
            local object_dump_ok, object_dump_error = pcall(function()
                DumpAllObjects()
            end)
            if not object_dump_ok then
                fail("dump_objects", object_dump_error)
                return
            end
            marker("ACCEPT:LUA_DUMP_OBJECTS_COMPLETED")

            marker("ACCEPT:LUA_CXX_HEADERS_REQUESTED")
            local cxx_headers_ok, cxx_headers_error = pcall(function()
                GenerateSDK()
            end)
            if not cxx_headers_ok then
                fail("cxx_headers", cxx_headers_error)
                return
            end
            marker("ACCEPT:LUA_CXX_HEADERS_COMPLETED")

            marker("ACCEPT:LUA_DUMP_USMAP_REQUESTED")
            local usmap_ok, usmap_error = pcall(function()
                DumpUSMAP()
            end)
            if not usmap_ok then
                fail("dump_usmap", usmap_error)
                return
            end
            marker("ACCEPT:LUA_DUMP_USMAP_COMPLETED")
        end)
    end)
end

local console_ok, console_error = pcall(function()
    RegisterConsoleCommandGlobalHandler("ue4ss_acceptance_probe", function()
        marker("ACCEPT:LUA_CONSOLE_INVOKED")
        return true
    end)
end)
if console_ok then
    marker("ACCEPT:LUA_CONSOLE_REGISTERED")
else
    fail("console_register", console_error)
end

local hook_ok, hook_error = pcall(function()
    RegisterHook("/Script/Engine.Actor:K2_GetActorLocation", function(Context)
        if has_run_probe then
            return
        end

        local actor = Context:get()
        if not actor or not actor:IsValid() then
            return
        end
        has_run_probe = true
        marker("ACCEPT:LUA_BLUEPRINT_HOOK")

        local actors = FindAllOf("Actor")
        if not actors or #actors == 0 then
            fail("find_all", "no live Actor instances")
            return
        end
        marker(string.format("ACCEPT:LUA_FIND_ALL count=%d", #actors))

        local read_ok, original_value = pcall(function()
            return actor:GetPropertyValue("bCanBeDamaged")
        end)
        if not read_ok or type(original_value) ~= "boolean" then
            fail("property_read", read_ok and "bCanBeDamaged was not boolean" or original_value)
            return
        end
        marker(string.format("ACCEPT:LUA_PROPERTY_READ value=%s", tostring(original_value)))

        local changed_value = not original_value
        local write_ok, write_error = pcall(function()
            actor:SetPropertyValue("bCanBeDamaged", changed_value)
        end)
        local verify_ok, observed_value = pcall(function()
            return actor:GetPropertyValue("bCanBeDamaged")
        end)
        pcall(function()
            actor:SetPropertyValue("bCanBeDamaged", original_value)
        end)

        if not write_ok or not verify_ok or observed_value ~= changed_value then
            fail("property_write", write_ok and (verify_ok and observed_value or "readback failed") or write_error)
            return
        end
        marker(string.format("ACCEPT:LUA_PROPERTY_WRITE value=%s", tostring(observed_value)))

        local restore_ok, restored_value = pcall(function()
            return actor:GetPropertyValue("bCanBeDamaged")
        end)
        if not restore_ok or restored_value ~= original_value then
            fail("property_restore", restore_ok and restored_value or "readback failed")
            return
        end
        marker(string.format("ACCEPT:LUA_PROPERTY_RESTORED value=%s", tostring(restored_value)))

        local name_ok, probe_name = pcall(function()
            return FName("UE4SSAcceptanceProbeObject", EFindName.FNAME_Add)
        end)
        if not name_ok then
            fail("fname", probe_name)
            return
        end
        marker(string.format("ACCEPT:LUA_FNAME value=%s", probe_name:ToString()))

        local construct_ok, constructed_object = pcall(function()
            local object_class = StaticFindObject("/Script/CoreUObject.Object")
            if not object_class or not object_class:IsValid() then
                error("UObject class was not found")
            end
            return StaticConstructObject(object_class, actor, probe_name)
        end)
        if not construct_ok or not constructed_object or not constructed_object:IsValid() then
            fail("static_construct_object", construct_ok and "constructed object was invalid" or constructed_object)
            return
        end
        marker("ACCEPT:LUA_STATIC_CONSTRUCT_OBJECT")

        schedule_dumpers()
    end)
end)
if not hook_ok then
    fail("blueprint_hook_register", hook_error)
else
    ExecuteInGameThread(function()
        if has_run_probe then
            return
        end

        local actors = FindAllOf("Actor")
        if not actors or #actors == 0 then
            fail("blueprint_hook_trigger", "no live Actor instances")
            return
        end

        local actor = nil
        for _, candidate in ipairs(actors) do
            if candidate and candidate:IsValid() then
                actor = candidate
                break
            end
        end
        if not actor then
            fail("blueprint_hook_trigger", "no valid Actor instance")
            return
        end

        local trigger_ok, trigger_error = pcall(function()
            actor:K2_GetActorLocation()
        end)
        if not trigger_ok then
            fail("blueprint_hook_trigger", trigger_error)
        end
    end)
end

marker("ACCEPT:LUA_MOD_STARTED")

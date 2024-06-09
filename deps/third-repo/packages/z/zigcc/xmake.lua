package("zigcc")
    set_kind("toolchain")
    set_homepage("https://www.ziglang.org/")
    set_description("Zig is a general-purpose programming language and toolchain for maintaining robust, optimal, and reusable software.")

    on_source(function (package)
        local jsonfile = os.tmpfile() .. ".json"
        import("net.http")
        http.download("https://ziglang.org/download/index.json", jsonfile)
        import("core.base.json")
        local json = json.loadfile(jsonfile)["master"]
        -- print(json)
        if is_host("windows") then
            -- print("Windows: " .. json["x86_64-windows"]["tarball"])
            package:set("urls", json["x86_64-windows"]["tarball"])
            package:add("versions", "v0.13.0", json["x86_64-windows"]["shasum"])
        elseif is_host("linux") then
            -- print("Linux: " .. json["x86_64-linux"]["tarball"])
            package:set("urls", json["x86_64-linux"]["tarball"])
            package:add("versions", "v0.13.0", json["x86_64-linux"]["shasum"])
        else
            print("Unsupported platform!")
        end
    end)

    on_install("@macosx", "@linux", "@windows", "@msys", "@bsd", function (package)
        print("Install?" .. package:installdir())
        os.cp("*", package:installdir())
        package:addenv("PATH", package:installdir())
    end)

    on_test(function (package)
        os.vrun("zig version")
    end)
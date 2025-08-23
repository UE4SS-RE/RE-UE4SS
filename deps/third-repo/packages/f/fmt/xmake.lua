package("fmt")
    set_homepage("https://fmt.dev")
    set_description("fmt is an open-source formatting library providing a fast and safe alternative to C stdio and C++ iostreams.")
    set_license("MIT")

    add_urls("https://github.com/fmtlib/fmt/archive/refs/tags/$(version).tar.gz",
             "https://github.com/fmtlib/fmt.git")
    add_versions("11.2.0", "40626af88bd7df9a5fb80be7b25ac85b122d6c21")

    on_install(function (package)
        import("package.tools.cmake")
        
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        table.insert(configs, "-DFMT_DOC=OFF")
        table.insert(configs, "-DFMT_TEST=OFF")
        
        -- Add /utf-8 flag for MSVC
        if package:is_plat("windows") then
            table.insert(configs, "-DCMAKE_CXX_FLAGS=/utf-8")
        end
        
        cmake.install(package, configs)
        
        -- Apply patches after installation to handle UE macro conflicts
        local ranges_h = path.join(package:installdir(), "include", "fmt", "ranges.h")
        if os.isfile(ranges_h) then
            local content = io.readfile(ranges_h)
            
            -- Check if patch has already been applied
            if content:find("FMT_UE_HAD_CHECK") then
                print("UE macro conflict patches already applied to fmt/ranges.h")
                return
            end
            
            io.writefile(ranges_h .. ".backup", io.readfile(ranges_h))
            
            -- Find the include guard or pragma once
            local insert_pos = content:find("#define FMT_RANGES_H_")
            if not insert_pos then
                insert_pos = content:find("#pragma once")
            end
            
            if insert_pos then
                -- Find the end of the line
                local line_end = content:find("\n", insert_pos)
                if line_end then
                    -- Insert the macro guard after the include guard
                    local guard_code = [[

// Macro conflict guards for 'check'
#ifdef check
  #pragma push_macro("check")
  #undef check
  #define FMT_UE_HAD_CHECK
#endif
]]
                    content = content:sub(1, line_end) .. guard_code .. content:sub(line_end + 1)
                    
                    -- Find the last #endif by reversing the string and finding the first occurrence
                    local reverse_content = content:reverse()
                    local reverse_endif = reverse_content:find("fidne#")
                    
                    if reverse_endif then
                        -- Convert back to forward position
                        local last_endif_pos = #content - reverse_endif - 5  -- -5 for length of "endif"
                        
                        local restore_code = [[

// Restore 'check' macro
#ifdef FMT_UE_HAD_CHECK
  #pragma pop_macro("check")
  #undef FMT_UE_HAD_CHECK
#endif
]]
                        -- Insert before the last endif
                        content = content:sub(1, last_endif_pos - 1) .. restore_code .. content:sub(last_endif_pos)
                    end
                    
                    io.writefile(ranges_h, content)
                    print("Applied UE macro conflict patches to fmt/ranges.h")
                end
            end
        end
    end)

    on_test(function (package)
        -- Set up proper compile flags for the test
        local configs = {languages = "c++20"}
        
        -- Add /utf-8 flag for MSVC
        if package:is_plat("windows") then
            configs.cxflags = "/utf-8"
        end
        
        assert(package:check_cxxsnippets({test = [[
            #include <fmt/format.h>
            #include <string>
            void test() {
                std::string s = fmt::format("{}", 42);
            }
        ]]}, {configs = configs}))
    end)
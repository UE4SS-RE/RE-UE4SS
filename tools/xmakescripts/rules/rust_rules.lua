-- This rule extends xmake's default rust rules in order to support
-- precision linking. This rule makes rust report which static libs it expects
-- to be linked against and those libs are propagated up to anything else that
-- wants to link to the created staticlib.
rule("rust.link")
    add_imports("core.project.depend")

    on_load(function(target)
        -- Rust expects the native-static-libs dir to exist before we run `rustc`.
        os.mkdir(target:dependir())

        -- Save any native libraries we compiled with so we can add them as links later.
        target:add("rcflags", "--print native-static-libs="..path.translate(path.join(target:dependir(), "native-libs.txt")))
    end)

    after_build(function(target)
        -- After building, we need to add the `native-libs.txt` file to the .d file.
        -- This allows xmake to detect if we need to rebuild if the native libs reported by rust
        -- have been regenerated/changed.
        local native_libs = path.join(target:dependir(), "native-libs.txt")

        if not os.exists(native_libs) then
            raise("Cargo target: %s did not export a list of native libs to link against. Try `xmake build --rebuild %s`", target:name(), target:name())
        end

        local dependinfo = depend.load(target:dependfile()) or {}
        table.append(dependinfo.files, native_libs)
        -- There might be a way to avoid manual deduping of the dependency files to achieve
        -- idempotency, but this should suffice for now. The list of files is small (two files for now).
        dependinfo.files = table.unique(dependinfo.files)
        depend.save(dependinfo, target:dependfile())

        -- Load the native lib list that was generated during rustc invocation.
        -- We add these native libs as links.
        local data = io.readfile(native_libs)
        local deps = data:split(" ", {plain = true})
        for _, lib in ipairs(deps) do
            -- Windows has another quirk where we have to manually link against the msvcrtd.
            -- Even though rust reports that it links against msvcrt, we have to link to the debug runtime.      
            if lib == "msvcrt.lib" then
                if os.is_host("windows") and target:get("runtimes") == "MDd" then
                    target:add("links", "msvcrtd.lib", {public = true})
                end
            else
                target:add("links", lib, {public = true})
            end
        end
    end)

-- This rule allows for generic pure-rust Cargo.toml projects to have their building
-- marshalled and managed entirely by the Cargo/rustc toolchain.
rule("cargo.build")
    set_extensions(".toml")
    add_imports("cargo.cargo_helpers", "core.project.depend")

    on_load(function (target)
        -- Get the user supplied project to build from the cargo workspace.
        local project_name = target:extraconf("rules", "cargo.build", "project_name")
        assert(project_name, "No project_name passed to the cargo.build rule.")

        -- Convert the supplied project name to the rust output format.
        -- ex. patternsleuth.lib -> libpatternsleuth.rlib
        if target:is_static() then
            target:set("basename", "lib" .. project_name:gsub("-", "_"))
            target:set("extension", ".rlib")
        end

        local is_debug = target:extraconf("rules", "cargo.build", "is_debug")

        local _, cargo_output_dir, _ = cargo_helpers.get_cargo_context(target, is_debug)

        -- Add framework dirs so any projects that add_deps() this target can get the proper rust flags.
        -- frameworks are in the format of `--extern cratename=dir/to/crate.rlib`
        target:add("frameworks", path.join(cargo_output_dir, target:filename()), {public = true})
        -- frameworkdirs are in the format of `-L dependency=dir/to/deps/`
        target:add("frameworkdirs", path.join(cargo_output_dir, "deps"), {public = true})
    end)

    on_build_file(function (target, sourcefile, opt)
        -- Windows has a quirk where rust will prefer to use the shipped .lib imports from the windows cargo packages.
        -- This results in rust reporting that we have to link to windows0.48.5, etc.
        -- Instead of having to parse the windows package dependencies and link to their pre-built .lib,
        -- we use the windows_raw_dylib to allow rust to resolve and and create the imports automatically.
        -- More background on this can be found at https://kennykerr.ca/rust-getting-started/understanding-windows-targets.html
        if(os.is_host("windows")) then
            os.setenv("RUSTFLAGS", "--cfg windows_raw_dylib")
        end

        -- Get the user supplied project to build from the cargo workspace.
        local project_name = target:extraconf("rules", "cargo.build", "project_name")
        local is_debug = target:extraconf("rules", "cargo.build", "is_debug")
        local cargo_dir, cargo_mode_dir, cargo_mode = cargo_helpers.get_cargo_context(target, is_debug)
        local targetfile = path.join(cargo_mode_dir, target:filename())
        
        local argv = {
            "rustc",
            "--manifest-path="..sourcefile,
            -- Only support the cargo lib type for now.
            "--lib",
            "--profile",
            cargo_mode,
            "-p",
            project_name,
            "--crate-type",
            "rlib",
            "--target-dir="..cargo_dir,
            "--quiet"
        }

        -- Add any cargo features if we specified them in add_rules().
        local features = target:extraconf("rules", "cargo.build", "features")
        if features then 
            table.append(argv, "-F", table.concat(features, ","))
        end

        -- Support the dry-run logic from xmake.
        import("core.base.option")
        local dryrun = option.get("dry-run")
        
        -- Parse cargo's dependent files list and add to this target's dependent files list.
        -- This allows us to precisely add any .rs files that should trigger a rebuild.
        local dependent_files = cargo_helpers.get_dependencies(path.join(cargo_mode_dir, target:basename().. ".d"))
        -- Detect any changes to Cargo.toml.
        table.join2(dependent_files, sourcefile)
        -- Detect any changes to the xxx.rlib target file.
        table.join2(dependent_files, path.join(cargo_mode_dir, target:filename()))

        -- The function in depend.on_changed() runs when xmake detects changes that will require a rebuild.
        -- The conditions that trigger a rebuild are defined in on_changed(function(), { Conditions }).
        depend.on_changed(function ()
            -- Ensure the user has cargo installed.
            import("lib.detect.find_tool")
            local cargo = find_tool("cargo")
            if not cargo then
                raise("cargo not found!")
            end

            import("utils.progress")
            progress.show(opt.progress or 0, "${color.build.object}compiling.cargo %s", sourcefile)
            
            if not dryrun then
                os.execv(cargo.program, argv)
            end
        end, {  dependfile = target:dependfile(target:targetfile()),
                lastmtime = os.mtime(targetfile),
                changed = target:is_rebuilt(),
                values = argv,
                files = dependent_files,
                dryrun = dryrun})  
   end)

   before_clean(function(target)
        local is_debug = target:extraconf("rules", "cargo.build", "is_debug")
        local cargo_dir, _, cargo_mode = cargo_helpers.get_cargo_context(target, is_debug)
        local project_name = target:extraconf("rules", "cargo.build", "project_name")
        
        import("lib.detect.find_tool")
        local cargo = find_tool("cargo")
        if not cargo then
            raise("cargo not found!")
        end

        -- We use the cargo clean command to clean cargo built targets instead of just nuking the deps folder.
        -- This allows for faster builds since we can safely cache most of the rust dependency crates. 
        for _, sourcefile in ipairs(target:sourcefiles()) do 
            local argv = {
                "clean",
                "-p",
                project_name,
                "--target-dir="..cargo_dir,
                "--manifest-path="..sourcefile,
                "--profile",
                cargo_mode
            }
            os.execv(cargo.program, argv)
        end
    end)

    -- Cargo manages linking when building .rlib files.
    -- Override the on_link function to give full control to this cargo rule.
    on_link(function(target) end)
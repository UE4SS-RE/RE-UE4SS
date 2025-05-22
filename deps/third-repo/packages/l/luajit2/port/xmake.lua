set_xmakever("2.3.4")
set_policy("build.across_targets_in_parallel", false)

add_rules("mode.debug", "mode.release")
if is_mode("release") then
    add_cflags("-fomit-frame-pointer")
    add_cflags("-fno-stack-protector")
end

option("nojit")
    set_default(false)
    set_showmenu(true)
    add_defines("LUAJIT_DISABLE_JIT", "LUAJIT_DISABLE_FFI")

option("fpu")
    set_default(true)
    set_showmenu(true)
    add_defines("LJ_ARCH_HASFPU=1", "LJ_ABI_SOFTFP=0")

option("gc64")
    set_default(false)
    set_showmenu(true)

rule("dasc")
    set_extensions(".dasc")
    before_build_file(function(target, sourcefile)

        local outputdir = target:objectdir()
        if not os.isdir(outputdir) then
            os.mkdir(outputdir)
        end
        local argv = {"dynasm/dynasm.lua", "-LN"}
        if is_arch("x64", "x86_64", "arm64", "arm64-v8a", "mips64") then
            -- 64bits pointer
            table.insert(argv, "-D")
            table.insert(argv, "P64")
        end
        if target:opt("fpu") then
            table.insert(argv, "-D")
            table.insert(argv, "FPU")
            table.insert(argv, "-D")
            table.insert(argv, "HFABI")
        end
        -- jit is not supported on ios
        if not target:opt("nojit") and not is_plat("iphoneos", "watchos") then
            table.insert(argv, "-D")
            table.insert(argv, "JIT")
            table.insert(argv, "-D")
            table.insert(argv, "FFI")
        end
        if is_plat("windows", "mingw") then
            table.insert(argv, "-D")
            table.insert(argv, "WIN")
        end
        table.insert(argv, "-o")
        table.insert(argv, path.join(outputdir, "buildvm_arch.h"))
        table.insert(argv, sourcefile)
        os.vrunv(target:dep("minilua"):targetfile(), argv)
        target:add("includedirs", outputdir, {public = true})
    end)

rule("luajit_h")
    before_build(function (target)

        -- STINKY: stop mutating source dir
        os.cd("$(projectdir)/src")

        if os.isdir("$(projectdir)/.git") then
            local ver = os.iorunv("git", {"show", "-s", "--format=%ct"})
            io.writefile("luajit_relver.txt", ver)
        else
            os.cp("$(projectdir)/.relver", "luajit_relver.txt")
        end
        os.vrunv(target:dep("minilua"):targetfile(), { "host/genversion.lua" })

        os.cd("-")
    end)

rule("buildvm")
    before_build_files(function (target, sourcebatch)

        local buildvm = target:dep("buildvm")
        local outputdir = buildvm:objectdir()
        if not os.isdir(outputdir) then
            os.mkdir(outputdir)
        end

        local buildvm_bin = buildvm:targetfile()
        local sourcefiles = sourcebatch.sourcefiles
        os.vrunv(buildvm_bin, {"-m", "bcdef", "-o", "src/lj_bcdef.h", unpack(sourcefiles)})
        os.vrunv(buildvm_bin, {"-m", "ffdef", "-o", "src/lj_ffdef.h", unpack(sourcefiles)})
        os.vrunv(buildvm_bin, {"-m", "libdef", "-o", "src/lj_libdef.h", unpack(sourcefiles)})
        os.vrunv(buildvm_bin, {"-m", "recdef", "-o", "src/lj_recdef.h", unpack(sourcefiles)})
        os.vrunv(buildvm_bin, {"-m", "vmdef", "-o", "src/jit/vmdef.lua", unpack(sourcefiles)})
        os.vrunv(buildvm_bin, {"-m", "folddef", "-o", "src/lj_folddef.h", "src/lj_opt_fold.c"})
        if is_plat("windows", "mingw") then
            local lj_vm_obj = path.join(outputdir, "lj_vm.obj")
            os.vrunv(buildvm_bin, {"-m", "peobj", "-o", lj_vm_obj})
            table.join2(target:objectfiles(), lj_vm_obj)
        else
            import("core.tool.compiler")
            local lj_vm_asm = path.join(outputdir, "lj_vm.S")
            local lj_vm_obj = path.join(outputdir, "lj_vm.o")
            local march
            if is_plat("macosx", "iphoneos", "watchos") then
                march = "machasm"
            else
                march = "elfasm"
            end
            os.vrunv(buildvm_bin, {"-m", march, "-o", lj_vm_asm})
            compiler.compile(lj_vm_asm, lj_vm_obj, {target = target})
            table.join2(target:objectfiles(), lj_vm_obj)
        end
    end)

function set_host_toolchains()
    -- only for cross-compliation
    if is_plat(os.host()) then
        return
    end
    local arch
    if is_arch("arm64", "arm64-v8a", "mips64", "x86_64") then
        arch = is_host("windows") and "x64" or "x86_64"
    else
        arch = is_host("windows") and "x86" or "i386"
    end
    set_plat(os.host())
    set_arch(arch)
end

target("minilua")
    set_kind("binary")
    set_default(false)
    add_files("src/host/minilua.c")
    set_host_toolchains()
    if is_host("windows") then
        add_defines("_CRT_SECURE_NO_DEPRECATE", "_CRT_STDIO_INLINE=__declspec(dllexport)__inline")
    end


target("buildvm")
    set_kind("binary")
    set_default(false)
    add_deps("minilua")
    add_rules("luajit_h", "dasc")
    add_options("nojit", "fpu")
    add_includedirs("src", {public = true})
    set_host_toolchains()
    add_files("src/host/buildvm*.c")
    add_defines("LUAJIT_ENABLE_LUA52COMPAT", {public = true})
    if is_host("windows") then
        add_defines("_CRT_SECURE_NO_DEPRECATE", "_CRT_STDIO_INLINE=__declspec(dllexport)__inline")
    end
    if is_arch("x86", "i386") then
        add_files("src/vm_x86.dasc")
        add_defines("LUAJIT_TARGET=LUAJIT_ARCH_X86", {public = true})
    elseif is_arch("x64", "x86_64") then
        if has_config("gc64") then
            add_defines("LUAJIT_ENABLE_GC64", {public = true})
            add_files("src/vm_x64.dasc")
        else
            -- @see https://github.com/xmake-io/xmake-repo/issues/1264
            add_files("src/vm_x86.dasc")
        end
        add_defines("LUAJIT_TARGET=LUAJIT_ARCH_X64", {public = true})
    elseif is_arch("arm64", "arm64-v8a") then
        add_files("src/vm_arm64.dasc")
        add_defines("LUAJIT_TARGET=LUAJIT_ARCH_ARM64", {public = true})
    elseif is_arch("arm.*") then
        add_files("src/vm_arm.dasc")
        add_defines("LUAJIT_TARGET=LUAJIT_ARCH_ARM", {public = true})
    elseif is_arch("mips64") then
        add_files("src/vm_mips64.dasc")
        add_defines("LUAJIT_TARGET=LUAJIT_ARCH_MIPS64", {public = true})
    elseif is_arch("mips") then
        add_files("src/vm_mips.dasc")
        add_defines("LUAJIT_TARGET=LUAJIT_ARCH_MIPS", {public = true})
    elseif is_arch("ppc") then
        add_files("src/vm_ppc.dasc")
        add_defines("LUAJIT_TARGET=LUAJIT_ARCH_PPC", {public = true})
    end
    if is_plat("macosx", "iphoneos", "watchos") then
        add_defines("LUAJIT_OS=LUAJIT_OS_OSX", {public = true})
    elseif is_plat("windows") then
        add_defines("LUAJIT_OS=LUAJIT_OS_WINDOWS", {public = true})
    elseif is_plat("linux", "android") then
        add_defines("LUAJIT_OS=LUAJIT_OS_LINUX", {public = true})
    elseif is_plat("bsd") then
        add_defines("LUAJIT_OS=LUAJIT_OS_BSD", {public = true})
    else
        add_defines("LUAJIT_OS=LUAJIT_OS_OTHER", {public = true})
    end
    before_build("@windows", "@msys", "@cygwin", function (target)
        if not is_arch("x86", "x64", "mips", "mips64") then
            -- @note we need fix `illegal zero-sized array` errors for msvc
            io.gsub("src/lj_jit.h", "  LJ_K32__MAX\n", "  LJ_K32__MAX=1\n")
            io.gsub("src/lj_jit.h", "  LJ_K64__MAX,\n", "  LJ_K64__MAX=1\n")
        end
    end)


if is_kind("static") or is_kind("shared") then

    target("luajit")
        set_kind("$(kind)")
        add_deps("buildvm")
        add_options("nojit", "fpu")
        if is_mode("debug") then
            add_defines("LUA_USE_ASSERT")
        end
        if is_kind("shared") and is_plat("windows") then
            add_defines("LUA_BUILD_AS_DLL")
        end
        add_defines("LUAJIT_ENABLE_LUA52COMPAT", {public = true})
        add_defines("_FILE_OFFSET_BITS=64", "LARGEFILE_SOURCE", {public = true})
        add_undefines("_FORTIFY_SOURCE", {public = true})
        add_includedirs("src", {public = true})
        add_headerfiles("src/*.h", {prefixdir = "luajit"})
        add_headerfiles("src/lua.hpp", {prefixdir = "luajit"})
        add_files("src/ljamalg.c")
        add_files("src/lib_base.c",
                  "src/lib_math.c",
                  "src/lib_bit.c",
                  "src/lib_string.c",
                  "src/lib_table.c",
                  "src/lib_io.c",
                  "src/lib_os.c",
                  "src/lib_package.c",
                  "src/lib_debug.c",
                  "src/lib_jit.c",
                  "src/lib_ffi.c",
                  "src/lib_buffer.c", {rules = {"buildvm", override = true}})

end


if is_kind("binary") then

    target("luajit_bin")
        set_kind("binary")
        add_deps("luajit")
        set_basename("luajit")
        add_files("src/luajit.c")
        add_options("nojit", "fpu")
        if is_plat("windows") then
            add_syslinks("advapi32", "shell32")
            if is_arch("x86") then
                add_ldflags("/subsystem:console,5.01")
            else
                add_ldflags("/subsystem:console,5.02")
            end
        elseif is_plat("android") then
            add_syslinks("m", "c")
        elseif is_plat("macosx") then
            add_ldflags("-all_load", "-pagezero_size 10000", "-image_base 100000000")
        elseif is_plat("mingw") then
            add_ldflags("-static-libgcc", {force = true})
        else
            add_syslinks("pthread", "dl", "m", "c")
        end

end

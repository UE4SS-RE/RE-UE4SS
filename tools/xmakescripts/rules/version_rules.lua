local MINIMUM_MSVC_TOOLSET_VERSION = "14.39"
local MINIMUM_RUST_VERSION = "1.73.0"

local function compare_versions(ver1, ver2)
    for i = 1, math.max(#ver1, #ver2) do
        local num1 = ver1[i] or 0
        local num2 = ver2[i] or 0
        if num1 < num2 then
            return false
        elseif num1 > num2 then
            return true
        end
    end
    return true  -- Versions are equal
end

local function parse_version(ver)
    if not ver then return nil end
    local version_nums = {}
    for num in ver:gmatch("(%d+)") do
        table.insert(version_nums, tonumber(num))
    end
    return version_nums
end

rule("ue4ss.check.minimum.version")
    add_deps("check.msvc.ver", "check.rust.ver")

-- This rule checks the MSVC version number and raises an error if it is below the minimum.
rule("check.msvc.ver")
    on_config(function (target)
        local msvc = target:toolchain("msvc")
        if msvc then
            local vctools_ver = msvc:config("vcvars").VCToolsVersion
            local parsed_vcvars_ver = parse_version(vctools_ver)

            -- Still print the version number even if we are allowing all versions
            local version_check = get_config("versionCheck")
            if not version_check then
                cprint("checking for Microsoft C/C++ Compiler Toolset version ... ${color.success}%s", vctools_ver)
                return
            end
            
            local minimum_version = parse_version(MINIMUM_MSVC_TOOLSET_VERSION)
            
            if not compare_versions(parsed_vcvars_ver, minimum_version) then
                cprint("checking for Microsoft C/C++ Compiler Toolset version ... ${color.failure}%s", vctools_ver)
                raise("${color.failure}your MSVC toolset version is too low, please update to this version or higher: (%s)", MINIMUM_MSVC_TOOLSET_VERSION)
            else
                cprint("checking for Microsoft C/C++ Compiler Toolset version ... ${color.success}%s", vctools_ver)
            end

            -- could also do this
            -- local vs_version = msvc:config("vcvars").VSCMD_VER
            -- if vs_version:find("pre") then
            --     cprint("${color.warning}you are using a preview version of Visual Studio, which may cause an error on build... consider downgrading to a stable version.")
            -- end
        end
    end)

rule("check.rust.ver")
    on_config(function(target)
        import("lib.detect.find_tool")
        local rustc = find_tool("rustc")
        if not rustc then
            raise("${color.failure}rustc not found! Please install Rust from https://www.rust-lang.org/tools/install")
        end

        local version = os.iorunv(rustc.program, {"--version"})
        local version_num = version:match("rustc (%d+.%d+.%d+)")
        if version_num then
            -- Still print the version number even if we are allowing all versions
            local version_check = get_config("versionCheck")
            if not version_check then
                cprint("checking for Rust version ... ${color.success}%s", version_num)
                return
            end

            if not compare_versions(parse_version(version_num), parse_version(MINIMUM_RUST_VERSION)) then
                cprint("checking for Rust version ... ${color.failure}%s", version_num)
                raise("${color.failure}your Rust version is too low, please update to this version or higher: (%s)", MINIMUM_RUST_VERSION)
            else
                cprint("checking for Rust version ... ${color.success}%s", version_num)
            end
        end
    end)
# Unified build script for RE-UE4SS on Windows
# Supports native and cross-compilation builds with various configurations

param(
    [Alias("g")]
    [string]$Generator = "",  # Default will be set based on what's available
    [Alias("c")]
    [string]$Compiler = "",
    [Alias("t")]
    [string]$Toolchain = "",
    [Alias("b")]
    [string]$BuildConfig = "Game__Shipping__Win64",
    [string]$Target = "UE4SS",
    [Alias("v")]
    [switch]$Verbose,
    [switch]$Clean,
    [Alias("h")]
    [switch]$Help
)

# Colors for output
function Write-Error-Custom { Write-Host "Error: $args" -ForegroundColor Red }
function Write-Success { Write-Host $args -ForegroundColor Green }
function Write-Info { Write-Host $args -ForegroundColor Yellow }

# Function to display usage
function Show-Usage {
    @"
Usage: .\build.ps1 [OPTIONS]

Options:
  -g, --generator GENERATOR    Build system generator (ninja, vs2022, vs2019) [default: auto-detect VS2022/VS2019, fallback to ninja]
  -c, --compiler COMPILER      Compiler to use (msvc, clang) [default: msvc when using ninja, VS default when using VS generator]
  -t, --toolchain TOOLCHAIN    CMake toolchain file name (without path/extension)
                               Example: windows-clang-cl
  -b, --build-config CONFIG    Build configuration [default: Game__Shipping__Win64]
  --target TARGET              Build target [default: UE4SS]
  -v, --verbose                Enable verbose output
  --clean                      Clean build directory before building
  -h, --help                   Display this help message
  
Note: PowerShell also accepts single dash (-) prefix for all parameters

Environment Variables:
  CMAKE_PREFIX_PATH       Additional CMake search paths

Examples:
  # Native build with system compiler
  .\build.ps1

  # Native build with Visual Studio 2022
  .\build.ps1 --generator vs2022

  # Build with clang-cl
  .\build.ps1 --compiler clang

  # Build specific configuration
  .\build.ps1 --build-config Game__Debug__Win64

  # Clean build with verbose output
  .\build.ps1 --clean --verbose
"@
    exit 0
}

if ($Help) { Show-Usage }

# Function to check if a tool is available
function Test-Tool {
    param([string]$Tool)
    
    $command = Get-Command $Tool -ErrorAction SilentlyContinue
    if (-not $command) {
        Write-Error-Custom "$Tool is not installed or not in PATH"
        return $false
    }
    return $true
}

# Get script and project directories
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = (Get-Item "$ScriptDir\..\..").FullName

# Check for CMake
if (-not (Test-Tool "cmake")) { exit 1 }

# Set default generator if not specified
if (-not $Generator) {
    # Check for Visual Studio installations first
    $VSWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $VSWhere) {
        $VSInstances = & $VSWhere -latest -property installationVersion
        if ($VSInstances) {
            $VSVersion = $VSInstances | Select-Object -First 1
            if ($VSVersion -like "17.*") {
                $Generator = "vs2022"
                Write-Info "Detected Visual Studio 2022, using as default generator"
            } elseif ($VSVersion -like "16.*") {
                $Generator = "vs2019"
                Write-Info "Detected Visual Studio 2019, using as default generator"
            }
        }
    }
    
    # Fall back to ninja if no VS found
    if (-not $Generator) {
        if (Get-Command "ninja" -ErrorAction SilentlyContinue) {
            $Generator = "ninja"
            Write-Info "Using Ninja as default generator"
            # When using ninja on Windows without a compiler specified, default to MSVC
            if (-not $Compiler -and -not $Toolchain) {
                $Compiler = "msvc"
                Write-Info "Using MSVC as default compiler with Ninja"
            }
        } else {
            Write-Error-Custom "No suitable build generator found. Please install Visual Studio or Ninja."
            exit 1
        }
    }
}

# Validate and convert generator to CMake format
$CMakeGenerator = switch ($Generator) {
    "ninja" { 
        if (-not (Test-Tool "ninja")) { exit 1 }
        "Ninja"
    }
    "vs2022" { "Visual Studio 17 2022" }
    "vs2019" { "Visual Studio 16 2019" }
    default {
        Write-Error-Custom "Invalid generator: $Generator"
        exit 1
    }
}

# Warn if using VS generator with clang compiler
if ($Compiler -eq "clang" -and $Generator -match "vs") {
    Write-Info "Note: Visual Studio generators work best with MSVC. For clang builds, consider using Ninja."
    Write-Info "Example: .\build.ps1 -g ninja -c clang"
}

# Set up build directory name
$BuildDirPrefix = "build"
if ($Toolchain) {
    $BuildDirPrefix = "${BuildDirPrefix}_${Toolchain}"
}
if ($Compiler) {
    $BuildDirPrefix = "${BuildDirPrefix}_${Compiler}"
}
$BuildDir = "${BuildDirPrefix}_${BuildConfig}"

# Set up CMake arguments
$CMakeArgs = @(
    $ProjectRoot,
    "-G", $CMakeGenerator,
    "-DCMAKE_BUILD_TYPE=$BuildConfig",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
)

# Add architecture for Visual Studio generators
if ($Generator -like "vs*") {
    $CMakeArgs += "-A", "x64"
    
    # For VS generators with clang, use ClangCL toolset
    if ($Compiler -eq "clang") {
        # Check if ClangCL toolset is available
        $ClangCLAvailable = $false
        $VSInstallDir = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath
        if ($VSInstallDir) {
            $ClangCLPath = Join-Path $VSInstallDir "VC\Tools\Llvm\x64\bin\clang-cl.exe"
            if (Test-Path $ClangCLPath) {
                $ClangCLAvailable = $true
            }
        }
        
        if ($ClangCLAvailable) {
            $CMakeArgs += "-T", "ClangCL"
            Write-Info "Using ClangCL toolset with Visual Studio generator"
        } else {
            Write-Info "ClangCL toolset not found in Visual Studio. Make sure you have installed the 'Clang compiler for Windows' component."
            Write-Info "Attempting to use system clang-cl instead..."
            # Don't set toolset, but specify compilers directly
            if (Test-Tool "clang-cl") {
                $CMakeArgs += "-DCMAKE_C_COMPILER=clang-cl", "-DCMAKE_CXX_COMPILER=clang-cl"
            } else {
                Write-Error-Custom "Neither VS ClangCL toolset nor system clang-cl found"
                exit 1
            }
        }
    }
}

# Set up compiler if specified
if ($Compiler) {
    switch ($Compiler) {
        "msvc" {
            # Explicitly set MSVC compiler
            if ($Generator -notlike "vs*") {
                if (Test-Tool "cl") {
                    $CMakeArgs += "-DCMAKE_C_COMPILER=cl", "-DCMAKE_CXX_COMPILER=cl"
                    Write-Info "Using MSVC (cl) compiler"
                } else {
                    # Try to find cl.exe in Visual Studio installation
                    $VSWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
                    if (Test-Path $VSWhere) {
                        $VSPath = & $VSWhere -latest -property installationPath
                        if ($VSPath) {
                            $CLPath = Get-ChildItem -Path "$VSPath\VC\Tools\MSVC" -Recurse -Filter "cl.exe" | 
                                       Where-Object { $_.FullName -like "*Hostx64\x64*" } | 
                                       Select-Object -First 1
                            if ($CLPath) {
                                $CMakeArgs += "-DCMAKE_C_COMPILER=`"$($CLPath.FullName)`"", "-DCMAKE_CXX_COMPILER=`"$($CLPath.FullName)`""
                                Write-Info "Using MSVC from: $($CLPath.DirectoryName)"
                            } else {
                                Write-Error-Custom "MSVC cl.exe not found in Visual Studio installation"
                                exit 1
                            }
                        }
                    } else {
                        Write-Error-Custom "MSVC not found. Please ensure Visual Studio is installed."
                        exit 1
                    }
                }
            }
        }
        "clang" {
            # For non-VS generators, specify compiler directly
            if ($Generator -notlike "vs*") {
                if (Test-Tool "clang-cl") {
                    $CMakeArgs += "-DCMAKE_C_COMPILER=clang-cl", "-DCMAKE_CXX_COMPILER=clang-cl"
                    Write-Info "Using clang-cl for native Windows build"
                } elseif (Test-Tool "clang") {
                    $CMakeArgs += "-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++"
                    Write-Info "Using clang/clang++ for build"
                } else {
                    Write-Error-Custom "Clang not found"
                    exit 1
                }
            }
        }
        default {
            Write-Error-Custom "Invalid compiler: $Compiler"
            exit 1
        }
    }
}

# Set up toolchain if specified
if ($Toolchain) {
    $ToolchainFile = "$ProjectRoot\cmake\toolchains\${Toolchain}-toolchain.cmake"
    if (-not (Test-Path $ToolchainFile)) {
        Write-Error-Custom "Toolchain file not found: $ToolchainFile"
        Write-Info "Available toolchains:"
        Get-ChildItem "$ProjectRoot\cmake\toolchains\" -Filter "*.cmake" | 
            ForEach-Object { "  " + $_.BaseName -replace '-toolchain$', '' }
        exit 1
    }
    $CMakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$ToolchainFile"
}

# Print build configuration
Write-Host "========================================"
Write-Info "Building RE-UE4SS"
Write-Host "========================================"
Write-Host "Generator:     $Generator"
Write-Host "Compiler:      $(if ($Compiler) { $Compiler } else { 'system default' })"
Write-Host "Toolchain:     $(if ($Toolchain) { $Toolchain } else { 'none' })"
Write-Host "Configuration: $BuildConfig"
Write-Host "Target:        $Target"
Write-Host "Build dir:     $BuildDir"
Write-Host "========================================"

# Clean build directory if requested
if ($Clean) {
    Write-Info "Cleaning build directory..."
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
        Write-Success "Build directory cleaned successfully!"
    } else {
        Write-Info "Build directory does not exist, nothing to clean"
    }
}

# Create and enter build directory
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
Push-Location $BuildDir

# Create .gitignore to prevent accidental commits of build artifacts
$GitIgnorePath = ".gitignore"
if (-not (Test-Path $GitIgnorePath)) {
    @"
# Automatically generated .gitignore for build directory
# This prevents accidental commits of build artifacts

# Ignore everything in this directory
*
"@ | Out-File -FilePath $GitIgnorePath -Encoding UTF8
    Write-Info "Created .gitignore in build directory"
}

try {
    # Configure with CMake
    Write-Info "Configuring with CMake..."
    if ($Verbose) {
        Write-Host "CMake command: cmake $($CMakeArgs -join ' ')"
    }

    & cmake $CMakeArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Error-Custom "CMake configuration failed!"
        exit 1
    }

    # Build
    Write-Info "Building target: $Target..."
    $BuildArgs = @("--build", ".", "--target", $Target)
    
    # For multi-configuration generators (Visual Studio), specify the configuration
    if ($Generator -match "vs") {
        $BuildArgs += "--config", $BuildConfig
    }
    
    if ($Verbose) {
        # The '--' argument tells cmake to pass subsequent flags to the native build tool (Ninja, MSBuild, etc.)
        $BuildArgs += "--"
        
        # Add the generator-specific verbosity flag
        $BuildArgs += switch ($Generator) {
            "ninja" { "-v" }
            default { "/verbosity:detailed" }
        }
    }

    & cmake $BuildArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Error-Custom "Build failed!"
        exit 1
    }
}
finally {
    # Return to original directory
    Pop-Location
}

Write-Host ""
Write-Success "========================================"
Write-Success "Build completed successfully!"
Write-Success "Configuration: $BuildConfig"
Write-Success "Output is in: $BuildDir"
if (Test-Path "$BuildDir\compile_commands.json") {
    Write-Success "Compile commands: $BuildDir\compile_commands.json"
}
Write-Success "========================================"
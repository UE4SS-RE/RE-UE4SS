# Unified build script for RE-UE4SS on Windows
# Supports native and cross-compilation builds with various configurations

param(
    [string]$Generator = "ninja",
    [string]$Compiler = "",
    [string]$Toolchain = "",
    [string]$BuildConfig = "Game__Shipping__Win64",
    [string]$Target = "UE4SS",
    [switch]$Verbose,
    [switch]$Clean,
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
  -Generator GENERATOR    Build system generator (ninja, vs2022, vs2019) [default: ninja]
  -Compiler COMPILER      Compiler to use (msvc, clang) [default: system default]
  -Toolchain TOOLCHAIN    CMake toolchain file name (without path/extension)
                          Example: windows-clang-cl
  -BuildConfig CONFIG     Build configuration [default: Game__Shipping__Win64]
  -Target TARGET          Build target [default: UE4SS]
  -Verbose                Enable verbose output
  -Clean                  Clean build directory before building
  -Help                   Display this help message

Environment Variables:
  CMAKE_PREFIX_PATH       Additional CMake search paths

Examples:
  # Native build with system compiler
  .\build.ps1

  # Native build with Visual Studio 2022
  .\build.ps1 -Generator vs2022

  # Build with clang-cl
  .\build.ps1 -Compiler clang

  # Build specific configuration
  .\build.ps1 -BuildConfig Game__Debug__Win64

  # Clean build with verbose output
  .\build.ps1 -Clean -Verbose
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

# Validate generator
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

# Get script and project directories
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = (Get-Item "$ScriptDir\..\..").FullName

# Check for CMake
if (-not (Test-Tool "cmake")) { exit 1 }

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
}

# Set up compiler if specified
if ($Compiler) {
    switch ($Compiler) {
        "msvc" {
            # Default for Windows
        }
        "clang" {
            if (Test-Tool "clang-cl") {
                $env:CC = "clang-cl"
                $env:CXX = "clang-cl"
            } elseif (Test-Tool "clang") {
                $env:CC = "clang"
                $env:CXX = "clang++"
            } else {
                Write-Error-Custom "Clang not found"
                exit 1
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
    }
}

# Create and enter build directory
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
Push-Location $BuildDir

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
    
    if ($Verbose) {
        $BuildArgs += "--", switch ($Generator) {
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
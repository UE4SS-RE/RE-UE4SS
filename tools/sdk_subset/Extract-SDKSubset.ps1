<#
.SYNOPSIS
    Extracts a few types and everything they need out of a generated UE4SS SDK tree.

.DESCRIPTION
    A generated SDK covers every reflected type in a game, which is far more than a mod needs and
    slow to compile. This copies the headers for the requested types plus their transitive include
    closure into a standalone tree with its own CMakeLists, so a mod can add_subdirectory() it and
    link UE4SS_SDK exactly as it would the full tree.

    Backend types (FName, TArray, UObject and friends) are not copied. They come from UE4SS itself,
    which the mod already links.

.PARAMETER TreeRoot
    The generated 'UE4SS_SDK' directory, the one containing src/ and CMakeLists.txt.

.PARAMETER Types
    Type names as spelled in C++, e.g. APalPlayerCharacter, FPalItemSlot, EPalWorkSuitability.

.PARAMETER Headers
    Include paths relative to UE4SS_SDK, e.g. Script/Pal/PalPlayerCharacter.hpp. An alternative to
    -Types when you already know the header.

.PARAMETER OutDir
    Where to write the subset. Created if missing, replaced if it exists.

.PARAMETER PointerDepth
    Object pointer members are forward-declared, so by default you get the requested types but
    cannot dereference their pointers. Each level of depth also extracts the types those pointers
    name. 1 is usually what you want; the closure grows quickly beyond that.

.PARAMETER WithLayoutAsserts
    Also emit LayoutAsserts.hpp filtered to the extracted types, so the subset can verify itself.

.EXAMPLE
    .\Extract-SDKSubset.ps1 -TreeRoot E:\trees\Palworld_run5\UE4SS_SDK -Types APalPlayerCharacter -OutDir E:\MyMod\PalSDK
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$TreeRoot,
    [string[]]$Types = @(),
    [string[]]$Headers = @(),
    [Parameter(Mandatory = $true)][string]$OutDir,
    [int]$PointerDepth = 0,
    [switch]$WithLayoutAsserts
)

$ErrorActionPreference = 'Stop'

$sourceRoot = Join-Path $TreeRoot 'src/UE4SS_SDK'
if (-not (Test-Path $sourceRoot)) { throw "Not a generated SDK tree (no src/UE4SS_SDK): $TreeRoot" }
if ($Types.Count -eq 0 -and $Headers.Count -eq 0) { throw 'Pass -Types and/or -Headers.' }

# Include paths are relative to the src directory and always use forward slashes, matching the
# '#include <UE4SS_SDK/...>' lines inside the headers.
$srcPrefix = (Resolve-Path (Join-Path $TreeRoot 'src')).Path.TrimEnd('\') + '\'
function ConvertTo-IncludePath([string]$fullPath) {
    return $fullPath.Substring($srcPrefix.Length).Replace('\', '/')
}

# A type index costs a full scan of the tree, so it is cached beside the tree and reused until the
# tree is regenerated.
$indexPath = Join-Path $TreeRoot '.sdk_type_index.json'
$treeStamp = (Get-Item (Join-Path $sourceRoot 'Macros.hpp')).LastWriteTimeUtc.Ticks
$index = $null
if (Test-Path $indexPath) {
    $cached = Get-Content $indexPath -Raw | ConvertFrom-Json
    if ($cached.Stamp -eq $treeStamp) {
        $index = @{}
        foreach ($property in $cached.Map.PSObject.Properties) { $index[$property.Name] = $property.Value }
        Write-Host "Using cached type index ($($index.Count) types)."
    }
}

if ($null -eq $index) {
    Write-Host "Indexing types in $sourceRoot (one-time, cached afterwards)..."
    $index = @{}
    $declaration = [regex]'(?m)^\s*(?:class|struct)\s+RC_UE4SS_SDK_API\s+(?:alignas\([^)]*\)\s+)?([A-Za-z_][A-Za-z0-9_]*)|^\s*enum(?:\s+class)?\s+([A-Za-z_][A-Za-z0-9_]*)|^\s*namespace\s+(?!WSDK)([A-Za-z_][A-Za-z0-9_]*)'
    foreach ($file in [System.IO.Directory]::EnumerateFiles($sourceRoot, '*.hpp', 'AllDirectories')) {
        $text = [System.IO.File]::ReadAllText($file)
        $include = ConvertTo-IncludePath $file
        foreach ($match in $declaration.Matches($text)) {
            foreach ($group in 1, 2, 3) {
                $name = $match.Groups[$group].Value
                if ($name -and -not $index.ContainsKey($name)) { $index[$name] = $include }
            }
        }
    }
    @{ Stamp = $treeStamp; Map = $index } | ConvertTo-Json -Depth 3 -Compress | Set-Content $indexPath -Encoding UTF8
    Write-Host "Indexed $($index.Count) types."
}

# Seed the walk from the requested types and headers.
$roots = [System.Collections.Generic.List[string]]::new()
foreach ($type in $Types) {
    if (-not $index.ContainsKey($type)) {
        $near = $index.Keys | Where-Object { $_ -like "*$type*" } | Select-Object -First 5
        $hint = ''
        if ($near) { $hint = ' Close matches: ' + ($near -join ', ') }
        throw "Type '$type' is not in this SDK.$hint"
    }
    $roots.Add($index[$type])
}
foreach ($header in $Headers) {
    $normalized = $header.Replace('\', '/').TrimStart('/')
    if (-not $normalized.StartsWith('UE4SS_SDK/')) { $normalized = "UE4SS_SDK/$normalized" }
    if (-not (Test-Path (Join-Path $srcPrefix $normalized))) { throw "Header not in this SDK: $normalized" }
    $roots.Add($normalized)
}

# Transitive closure over the generated includes. Backend includes are left alone.
$sdkInclude = [regex]'#include\s*<\s*(UE4SS_SDK/[^>]+)\s*>'
$needed = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$queue = [System.Collections.Generic.Queue[string]]::new()
foreach ($root in $roots) { if ($needed.Add($root)) { $queue.Enqueue($root) } }

while ($queue.Count -gt 0) {
    $current = $queue.Dequeue()
    $currentPath = Join-Path $srcPrefix $current
    if (-not (Test-Path $currentPath)) { continue }
    foreach ($match in $sdkInclude.Matches([System.IO.File]::ReadAllText($currentPath))) {
        $dependency = $match.Groups[1].Value
        if ($needed.Add($dependency)) { $queue.Enqueue($dependency) }
    }
}

# Pointer members are emitted as forward declarations, so their types are absent from the include
# closure. Pull them in when asked, one level at a time, so the growth stays visible.
$forwardDeclaration = [regex]'(?m)^\s*class\s+([A-Za-z_][A-Za-z0-9_]*)\s*;'
for ($level = 1; $level -le $PointerDepth; $level++) {
    $before = $needed.Count
    foreach ($include in @($needed)) {
        $includePath = Join-Path $srcPrefix $include
        if (-not (Test-Path $includePath)) { continue }
        foreach ($match in $forwardDeclaration.Matches([System.IO.File]::ReadAllText($includePath))) {
            $declared = $match.Groups[1].Value
            if (-not $index.ContainsKey($declared)) { continue }
            $queue.Enqueue($index[$declared]) | Out-Null
            $null = $needed.Add($index[$declared])
        }
    }
    while ($queue.Count -gt 0) {
        $current = $queue.Dequeue()
        $currentPath = Join-Path $srcPrefix $current
        if (-not (Test-Path $currentPath)) { continue }
        foreach ($match in $sdkInclude.Matches([System.IO.File]::ReadAllText($currentPath))) {
            $dependency = $match.Groups[1].Value
            if ($needed.Add($dependency)) { $queue.Enqueue($dependency) }
        }
    }
    Write-Host "Pointer depth $level : $before -> $($needed.Count) headers."
}

# Macros.hpp defines the API macro and the layout-storage templates every header relies on.
$null = $needed.Add('UE4SS_SDK/Macros.hpp')

Write-Host "$($roots.Count) requested type(s) pull in $($needed.Count) headers."

if (Test-Path $OutDir) { Remove-Item $OutDir -Recurse -Force }
$outSource = Join-Path $OutDir 'src'
foreach ($include in $needed) {
    $from = Join-Path $srcPrefix $include
    if (-not (Test-Path $from)) { continue }
    $to = Join-Path $outSource $include
    $null = New-Item -ItemType Directory -Force (Split-Path $to)
    Copy-Item $from $to -Force
}

# A master header covering every copied header. Types reached through pointer expansion are not
# included by anything else, so listing only the requested roots would leave them declared but
# undefined.
$masterIncludes = ($needed |
    Where-Object { $_ -notmatch '/(Include|LayoutAsserts|RuntimeSDKTest)\.hpp$' } |
    Sort-Object -Unique |
    ForEach-Object { "#include <$_>" }) -join [Environment]::NewLine
$includeText = @'
#pragma once

// Generated by Extract-SDKSubset.ps1.

'@
$includeText + $masterIncludes + [Environment]::NewLine | Set-Content (Join-Path $outSource 'UE4SS_SDK/Include.hpp') -Encoding UTF8

if ($WithLayoutAsserts) {
    $assertsPath = Join-Path $sourceRoot 'LayoutAsserts.hpp'
    if (Test-Path $assertsPath) {
        # Keep only asserts whose every referenced type was extracted, so the subset still verifies
        # itself without dragging in the rest of the SDK.
        $extracted = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
        foreach ($entry in $index.GetEnumerator()) {
            if ($needed.Contains($entry.Value)) { $null = $extracted.Add($entry.Key) }
        }
        $kept = [System.Collections.Generic.List[string]]::new()
        $assertType = [regex]'WSDK::([A-Za-z_][A-Za-z0-9_]*)'
        foreach ($line in [System.IO.File]::ReadAllLines($assertsPath)) {
            if ($line -notmatch 'static_assert') { continue }
            $referenced = $assertType.Matches($line) | ForEach-Object { $_.Groups[1].Value }
            if (-not $referenced) { continue }
            $missing = $referenced | Where-Object { -not $extracted.Contains($_) }
            if (-not $missing) { $kept.Add($line) }
        }
        $assertHeader = @'
#pragma once

#include <UE4SS_SDK/Include.hpp>

'@
        $assertHeader + ($kept -join [Environment]::NewLine) + [Environment]::NewLine |
            Set-Content (Join-Path $outSource 'UE4SS_SDK/LayoutAsserts.hpp') -Encoding UTF8
        Write-Host "Kept $($kept.Count) layout asserts for the extracted types."
    }
}

$cmakeText = @'
cmake_minimum_required(VERSION 3.18)

set(TARGET UE4SS_SDK)
project(${TARGET})

add_library(${TARGET} INTERFACE)
target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
'@
$cmakeText | Set-Content (Join-Path $OutDir 'CMakeLists.txt') -Encoding UTF8

$requested = ($Types + $Headers) -join ', '
$readmeText = @"
Subset of a generated UE4SS SDK.

Extracted for: $requested
Headers: $($needed.Count)

Use it from a C++ mod, alongside the mod's own CMakeLists:

    add_subdirectory("PalSDK" "`${CMAKE_CURRENT_BINARY_DIR}/sdk")
    target_link_libraries(YourMod PRIVATE UE4SS UE4SS_SDK)

then include what you need:

    #include <UE4SS_SDK/Include.hpp>

If one translation unit pulls in a large part of the SDK, MSVC needs /bigobj:

    if(MSVC)
        target_compile_options(YourMod PRIVATE /bigobj)
    endif()

Backend types (UObject, FName, TArray, ...) are not copied here. They come from UE4SS itself, which
the mod already links.

This subset matches the game build it was generated from. Regenerate after a game update.
To add another type later, re-run Extract-SDKSubset.ps1 with the full -Types list.
"@
$readmeText | Set-Content (Join-Path $OutDir 'README.txt') -Encoding UTF8

Write-Host "Subset written to $OutDir"

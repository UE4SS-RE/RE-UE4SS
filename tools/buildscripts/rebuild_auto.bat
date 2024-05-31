@echo off

set BuildMode=%1
set TargetName=%2

IF NOT DEFINED %BuildMode (
set BuildMode=Release
)

IF NOT DEFINED %TargetName (
set TargetName=ue4ss
)

set IsBuildConfigValid=1

IF NOT %BuildMode% == Release (
    IF NOT %BuildMode% == Debug (
        set IsBuildConfigValid=0
        echo Build mode must be either Release or Debug, not %BuildMode%.
    )
)

if NOT %TargetName% == ue4ss (
    if NOT %TargetName% == xinput1_3 (
        set IsBuildConfigValid=0
        echo Target name must be either ue4ss or xinput1_3, not %TargetName%
    )
)

IF %IsBuildConfigValid% == 1 (
    echo Target Name: %TargetName%
    echo Build Mode: %BuildMode%

    call internal_generate_build_files.bat %BuildMode%
    MSBuild.exe /m "VS_Solution\%TargetName%.vcxproj" /property:Configuration=%BuildMode% /t:Clean,Build
) else (
    echo Could not build, the build configuration is invalid.
    pause
    exit /b
)
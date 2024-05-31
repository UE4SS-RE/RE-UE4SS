:: Case-preserving builds have been turned off because we no longer support them widely.
:: set CASE_PRESERVING=1
:: call internal_build_tools/package_binary.bat "UE4SS - XInput - Case Preserving" "xinput1_3"

:: set CASE_PRESERVING=1
:: call internal_build_tools/package_binary.bat "UE4SS - Standard - Case Preserving" "ue4ss"

set CASE_PRESERVING=0
call internal_build_tools/package_binary.bat "UE4SS - Standard" "ue4ss"

:: Cleanup
call internal_build_tools/cleanup_build_option_files.bat

:: Set version numbers -> After compiling
if %IS_MINOR_RELEASE%==1 (
    set /a minor_version=%minor_version%+1
)
if %IS_MAJOR_RELEASE%==1 (
    set /a major_version=%major_version%+1
)
call internal_build_tools/set_version.bat %major_version% %minor_version% %hotfix_version% %pre_release_version% %beta_version%

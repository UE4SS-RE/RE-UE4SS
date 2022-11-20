@echo off
set IS_MAJOR_RELEASE=0
set IS_MINOR_RELEASE=0
set IS_HOTFIX_RELEASE=1
set IS_PRE_RELEASE=0
set IS_BETA_RELEASE=0

call internal_build_tools/packaging_header.bat

Rem LogLevel can be VERBOSE or NORMAL
set LOG_LEVEL=NORMAL

echo Packaging %major_version%.%minor_version%.%hotfix_version%

call internal_build_tools/package_all_distributions.bat

echo Done.
pause

set build_variant=%1%
if not defined build_variant (
    echo Was unable to call 'package_binary' because no build variant was passed
    pause
    exit
)

set file_to_package=%2%
if not defined file_to_package (
    echo Was unable to call 'package_binary' because no binary file was specified
    pause
    exit
)

if not defined IS_MAJOR_RELEASE (
    echo IS_MAJOR_RELEASE must be defined
    pause
    exit
)

if not defined IS_MINOR_RELEASE (
    echo IS_MINOR_RELEASE must be defined
    pause
    exit
)

if not defined IS_HOTFIX_RELEASE (
    echo IS_HOTFIX_RELEASE must be defined
    pause
    exit
)

if not defined IS_PRE_RELEASE (
    echo IS_PRE_RELEASE must be defined
    pause
    exit
)

if not defined IS_BETA_RELEASE (
    echo IS_BETA_RELEASE must be defined
    pause
    exit
)

:: String builder for final packaged filename -> START
set packaged_filename=UE4SS_Standard

if %CASE_PRESERVING% == 1 (
    set packaged_filename=%packaged_filename%_CasePreserving
)

set packaged_filename=%packaged_filename%_v%major_version%.%minor_version%.%hotfix_version%

if %IS_PRE_RELEASE% == 1 (
    set packaged_filename=%packaged_filename%_PreRelease%pre_release_version%
)

if %IS_BETA_RELEASE% == 1 (
    set packaged_filename=%packaged_filename%_Beta%beta_version%
)

set packaged_filename=%packaged_filename%.zip
:: String builder for final packaged filename -> END

Rem Create or remove build option files
echo Packaging %build_variant%...
call internal_build_tools/set_build_switches.bat %build_variant%

Rem Maintain the same build number across all binaries
Rem if %LOG_LEVEL%==VERBOSE echo Maintaining build number (TODO)

Rem Re-run CMake to apply build option files
call internal_build_tools/reload_cmake.bat

Rem Build project
if %LOG_LEVEL%==VERBOSE echo Building...
MSBuild.exe /m "VS_Solution\ue4ss.vcxproj" /property:Configuration=Release > nul

Rem Copy files to staging directory
if %LOG_LEVEL%==VERBOSE echo Staging binaries
xcopy /v /y "%build_output_directory%\%file_to_package%.dll" "%staging_directory%\" > nul

if %IS_BETA_RELEASE%==1 (
    xcopy /v /y "%build_output_directory%\%file_to_package%.pdb" "%staging_directory%\" > nul
)

Rem Zip files (7zip) and give zip a proper name
if %LOG_LEVEL%==VERBOSE echo Creating archive
cd %staging_directory%
%zip_exe% a "%packaged_filename%" "*" > nul
move %packaged_filename% ../%output_directory% > nul
cd ..

Rem Remove binaries from staging area but leave the rest
if %LOG_LEVEL%==VERBOSE echo Cleaning up staging directory
del %staging_directory%\%file_to_package%.dll

if %IS_BETA_RELEASE%==1 (
    del %staging_directory%\%file_to_package%.pdb
)

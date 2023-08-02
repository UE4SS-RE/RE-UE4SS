if not exist "VS_Solution\" (
	mkdir "VS_Solution"
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

Rem Required external tools
set zip_exe="C:\Program Files\7-Zip\7z.exe"

set enable_with_case_preserving_file=enable_with_case_preserving.build_option
set disable_with_case_preserving_file=disable_with_case_preserving.build_option
set build_file=VS_Solution\ue4ss_rewritten.sln
set build_output_directory=Output\ue4ss\Binaries\x64\Release
set staging_directory=Staging
set output_directory=Releases
set version_cache_file=version.cache

if not exist %staging_directory% (
    echo Staging directory, '%staging_directory%', does not exist.
    pause
    exit /b
)

if not exist generated_src (
    echo Unable to find project directory, 'generated_src'.
    pause
    exit /b
)

if not exist generated_src\version.cache (
    echo Unable to find version cache file, 'generated_src\version.cache'.
    pause
    exit /b
)

if not exist %output_directory% (
    mkdir %output_directory%
)

Rem Retrieve version numbers
set major_version=0
set minor_version=0
set hotfix_version=0
set pre_release_version=0
set beta_version=0
for /f "tokens=1,2,3,4,5 delims=." %%a in (generated_src\version.cache) do (
    set major_version=%%a
    set minor_version=%%b
    set hotfix_version=%%c
    set pre_release_version=%%d
    set beta_version=%%e
)

:: Set version numbers -> Before compiling
if %IS_PRE_RELEASE%==1 (
    set /a pre_release_version=%pre_release_version%+1
    set beta_version=0
)
if %IS_BETA_RELEASE%==1 (
    set /a beta_version=%beta_version%+1
)
if %IS_HOTFIX_RELEASE%==1 (
    set /a hotfix_version=%hotfix_version%+1
    set pre_release_version=0
    set beta_version=0
)
if %IS_MINOR_RELEASE%==1 (
    set hotfix_version=0
    set pre_release_version=0
    set beta_version=0
)
if %IS_MAJOR_RELEASE%==1 (
    set minor_version=0
    set hotfix_version=0
    set pre_release_version=0
    set beta_version=0
)
call internal_build_tools/set_version.bat %major_version% %minor_version% %hotfix_version% %pre_release_version% %beta_version%

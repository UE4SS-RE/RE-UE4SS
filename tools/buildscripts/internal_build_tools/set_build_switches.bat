set param_one=%1%
if not defined param_one (
    echo Was unable to call 'set_build_switches' because no build variant was passed
    pause
    exit
)

if %LOG_LEVEL%==VERBOSE echo Setting up build switches for %~1%

cd generated_src

if %LOG_LEVEL%==VERBOSE (
    echo generated src directory contents...
    dir /b /a-d
)

Rem Case Preserving - START
if exist %enable_with_case_preserving_file% (
    if %LOG_LEVEL%==VERBOSE echo Deleting 'enable_with_case_preserving_file'
    del "%enable_with_case_preserving_file%"
)

if exist %disable_with_case_preserving_file% (
    if %LOG_LEVEL%==VERBOSE echo Deleting 'disable_with_case_preserving_file'
    del "%disable_with_case_preserving_file%"
)

if %CASE_PRESERVING% == 1 (
    if %LOG_LEVEL%==VERBOSE echo Creating 'enable_with_case_preserving_file'
    type NUL > %enable_with_case_preserving_file%
) else (
    if %LOG_LEVEL%==VERBOSE echo Creating 'disable_with_case_preserving_file'
    type NUL > %disable_with_case_preserving_file%
)
Rem Case Preserving - END

cd ..

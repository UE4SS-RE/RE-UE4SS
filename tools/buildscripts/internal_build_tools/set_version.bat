set set_version_param_major=%1%
if not defined set_version_param_major (
    echo Was unable to call 'set_version' because no major version was passed
    pause
    exit
)

set set_version_param_minor=%2%
if not defined set_version_param_minor (
    echo Was unable to call 'set_version' because no minor version was passed
    pause
    exit
)

set set_version_param_hotfix=%3%
if not defined set_version_param_hotfix (
    echo Was unable to call 'set_version' because no hotfix version was passed
    pause
    exit
)

set set_version_param_prerelease=%4%
if not defined set_version_param_prerelease (
    echo Was unable to call 'set_version' because no pre-release version was passed
    pause
    exit
)

set set_version_param_beta=%5%
if not defined set_version_param_beta (
    echo Was unable to call 'set_version' because no beta version was passed
    pause
    exit
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

echo Setting version to: %set_version_param_major%.%set_version_param_minor%.%set_version_param_hotfix%.%set_version_param_prerelease%.%set_version_param_beta%

Rem Caching the incremented version
set full_version_string=%set_version_param_major%.%set_version_param_minor%.%set_version_param_hotfix%.%set_version_param_prerelease%.%set_version_param_beta%
echo %full_version_string%> generated_src\version.cache

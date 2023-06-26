set BuildMode=%1
set Compiler=%2

IF NOT DEFINED %BuildMode (
set BuildMode=Release
)

IF NOT DEFINED %Compiler (
set Compiler=MSVC
)

if %Compiler% == MSVC (
    if not exist "../../VS_Solution" (
        mkdir "../../VS_Solution"
    )
    
    cd internal_build_tools
    call generate_vs_solution_internal.bat %BuildMode%
)

if %Compiler% == ClangNinja (
    if not exist "../../ClangNinja" (
        mkdir "../../ClangNinja"
    )
    
    cd internal_build_tools
    call generate_clang_ninja_internal.bat %BuildMode%
)

cd ..

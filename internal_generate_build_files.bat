set BuildMode=%1

IF NOT DEFINED %BuildMode (
set BuildMode=Release
)

if not exist "VS_Solution\" (
	mkdir "VS_Solution"
)

cd internal_build_tools
call generate_vs_solution_internal.bat %BuildMode%
cd ..

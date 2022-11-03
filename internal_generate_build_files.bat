set BuildMode=%1

IF NOT DEFINED %BuildMode (
set BuildMode=Release
)

if not exist "VS_Solution\" (
	mkdir "VS_Solution"
)

cd VS_Solution
call generate_vs_solution.bat %BuildMode%
cd ..

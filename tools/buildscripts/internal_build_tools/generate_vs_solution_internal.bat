set BuildMode=%1

IF NOT DEFINED %BuildMode (
set BuildMode=Release
)

cd ../../../VS_Solution
cmake -DCMAKE_BUILD_TYPE=%BuildMode% -DRC_FORCE_ALL_STATIC_LIBS= -G"Visual Studio 17 2022" ..

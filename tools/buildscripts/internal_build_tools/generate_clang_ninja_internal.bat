set BuildMode=%1

IF NOT DEFINED %BuildMode (
set BuildMode=Release
)

cd ../../../ClangNinja
cmake -DCMAKE_BUILD_TYPE=%BuildMode% -DRC_FORCE_ALL_STATIC_LIBS= -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G"Ninja" ..

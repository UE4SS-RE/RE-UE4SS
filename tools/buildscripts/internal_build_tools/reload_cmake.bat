if %LOG_LEVEL%==VERBOSE echo Reloading CMake
cd VS_Solution
cmake -DCMAKE_BUILD_TYPE=Release -DRC_FORCE_ALL_STATIC_LIBS= -G"Visual Studio 16 2019" .. > nul 2>&1
cd ..
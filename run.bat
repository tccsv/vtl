cmake -S . -B build -A x64 -G "Visual Studio 17 2022" -D CMAKE_BUILD_TYPE=Release 
cmake --build build --config Release -j8
ctest --test-dir build --build-config Release --output-on-failure
app\Release\VTL.exe
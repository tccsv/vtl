cmake -S . -B build -A x64
cmake --build build --config Release -j8
ctest --test-dir build --build-config Release --output-on-failure
app\VTL.exe

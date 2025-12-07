Linux/macOS (using system dependencies)

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .


Windows (using VCPKG)

mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
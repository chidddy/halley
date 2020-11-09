@echo off

set targetDllPath="%~1\bin\SDL2.dll"
if not exist %targetDllPath% (
    echo Copying SDL2.dll to %targetDllPath%
    copy SDL2.dll %targetDllPath% || exit /b 1
)

cd /d %1
if not exist build mkdir build
cd build

set slnName="%~2.sln"
set binTargetName="%~2-gamebins"

IF NOT EXIST %slnName% (
cmake -G "Visual Studio 16 2019" ^
    -A x64 ^
    -DHALLEY_PATH=../halley ^
    -DBUILD_HALLEY_TOOLS=0 ^
    -DBUILD_HALLEY_TESTS=0 ^
    -DCMAKE_INCLUDE_PATH="lib\include" ^
    -DCMAKE_LIBRARY_PATH="lib\windows64" ^
    -DShaderConductor_LIBRARY="%ShaderConductorPath%\build\Lib\Debug\ShaderConductor.lib" ^
    -DShaderConductor_INCLUDE_DIR="%ShaderConductorPath%\Include" ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKGPath%/scripts/buildsystems/vcpkg.cmake" ^
    -DBoost_USE_STATIC_LIBS=1 ^
    .. || exit /b 1
)

cmake.exe --build . --target %binTargetName% --config %3 || exit /b 1

echo Build successful.
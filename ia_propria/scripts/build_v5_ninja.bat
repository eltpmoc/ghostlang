@echo off
call "E:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
set CUDA_PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6
set PATH=%CUDA_PATH%\bin;%PATH%

cd /d "%~dp0\..\.."
mkdir build_win_v5
cd build_win_v5

del CMakeCache.txt
rmdir /s /q CMakeFiles

cmake -G "Ninja" -S .. -B . -DGHOST_CUDA=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_CUDA_COMPILER="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.6/bin/nvcc.exe" -DCMAKE_CUDA_FLAGS="-allow-unsupported-compiler"

ninja ghost
if errorlevel 1 exit /b 1

cd /d "%~dp0\..\.."
mkdir ia_propria\build\v5
echo int main() { return 0; } > ia_propria\build\v5\ia.exe_temp.cpp

.\build_win_v5\ghost.exe -x ia_propria\v5\src\main.g -o ia_propria\build\v5\ia.exe

cd build_win_v5
ninja ia_propria_v5
if errorlevel 1 exit /b 1

copy /y ia_propria_v5.exe ..\ia_propria\build\v5\ia.exe
echo COMPILACAO CONCLUIDA COM NINJA E CUDA 12.6!

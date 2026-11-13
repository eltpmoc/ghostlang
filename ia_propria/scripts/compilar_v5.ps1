$ErrorActionPreference = "Stop"

$PROJECT = "$PSScriptRoot\.."
$ROOT = "$PROJECT\.."
$GHOST_BUILD_DIR = "$ROOT\build_win_v5"

Write-Host "Configurando CMake com MSVC e toolset 14.44..."

if (Test-Path "$GHOST_BUILD_DIR\CMakeCache.txt") {
    Remove-Item -Force "$GHOST_BUILD_DIR\CMakeCache.txt"
}
if (Test-Path "$GHOST_BUILD_DIR\CMakeFiles") {
    Remove-Item -Recurse -Force "$GHOST_BUILD_DIR\CMakeFiles"
}

New-Item -ItemType Directory -Force -Path "$PROJECT\build\v5" | Out-Null
Set-Content -Path "$PROJECT\build\v5\ia.exe_temp.cpp" -Value "int main() { return 0; }"

cmake -G "Visual Studio 18 2026" -T "v143" -S "$ROOT" -B "$GHOST_BUILD_DIR" -DGHOST_CUDA="ON" -DCMAKE_BUILD_TYPE=Release

if ($LASTEXITCODE -ne 0) {
    Write-Host "Caindo para toolset padrao..."
    if (Test-Path "$GHOST_BUILD_DIR\CMakeCache.txt") { Remove-Item -Force "$GHOST_BUILD_DIR\CMakeCache.txt" }
    cmake -G "Visual Studio 18 2026" -S "$ROOT" -B "$GHOST_BUILD_DIR" -DGHOST_CUDA="ON" -DCMAKE_BUILD_TYPE=Release
}

Write-Host "Compilando runtime C++..."
cmake --build "$GHOST_BUILD_DIR" --target ghost --config Release

Set-Location $PROJECT
New-Item -ItemType Directory -Force -Path "checkpoints\v5" | Out-Null

Write-Host "Compilando código GhostLang (main.g)..."
$ghostExe = "$GHOST_BUILD_DIR\Release\ghost.exe"

try {
    & $ghostExe -x v5/src/main.g -o build/v5/ia.exe
} catch {}

Write-Host "Compilando C++ nativo para Windows (MSVC)..."
cmake --build "$GHOST_BUILD_DIR" --target ia_propria_v5 --config Release

if (Test-Path "$GHOST_BUILD_DIR\Release\ia_propria_v5.exe") {
    Copy-Item -Force "$GHOST_BUILD_DIR\Release\ia_propria_v5.exe" "$PROJECT\build\v5\ia.exe"
}

Write-Host "V5 compilada nativamente para Windows (ia.exe)!"

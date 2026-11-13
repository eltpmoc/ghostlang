# build_quantum_cuda.ps1 - Compila o backend CUDA do simulador quantico (ghost_quantum_cuda.dll).
#
# O runtime da GhostLang e compilado com MinGW; o nvcc no Windows exige MSVC. Por isso o backend e uma
# DLL com API em C, carregada em tempo de execucao (sem a DLL, o simulador roda na CPU).
#
# Uso: powershell -ExecutionPolicy Bypass -File scripts\build\build_quantum_cuda.ps1 [-Cuda v12.9] [-Arch "61;75;86;89"]
param(
    [string]$Cuda = "",                 # versao do toolkit (ex.: v12.9). Vazio = a mais nova que ainda gera sm_61, se houver
    [string]$Arch = "61;75;86;89",      # arquiteturas (sm_XX); a ultima tambem vai como PTX
    [string]$MsvcVersion = ""           # toolset MSVC (ex.: 14.44). Vazio = o mais novo 14.3x/14.4x (suportado pelo CUDA 12)
)
$ErrorActionPreference = "Stop"
$root = Resolve-Path "$PSScriptRoot\..\.."
$src = Join-Path $root "src\cuda\GhostQuantumCuda.cu"
$out = Join-Path $root "bin\ghost_quantum_cuda.dll"

# 1. Toolkit CUDA
$cudaBase = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA"
if (-not $Cuda) {
    # CUDA 13+ nao gera mais codigo para Pascal (sm_61); prefere o 12.x mais novo
    $v12 = Get-ChildItem $cudaBase -Directory | Where-Object { $_.Name -like "v12.*" } | Sort-Object { [version]($_.Name.TrimStart('v')) } | Select-Object -Last 1
    $any = Get-ChildItem $cudaBase -Directory | Sort-Object { [version]($_.Name.TrimStart('v')) } | Select-Object -Last 1
    $Cuda = if ($v12) { $v12.Name } else { $any.Name }
}
$nvcc = Join-Path $cudaBase "$Cuda\bin\nvcc.exe"
if (-not (Test-Path $nvcc)) { throw "nvcc nao encontrado: $nvcc" }

# 2. Visual Studio e toolset MSVC
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -all -prerelease -products * -property installationPath | Select-Object -First 1
if (-not $vs) { throw "Visual Studio (C++) nao encontrado" }
$vcvars = Join-Path $vs "VC\Auxiliary\Build\vcvars64.bat"
if (-not $MsvcVersion) {
    $tools = Get-ChildItem (Join-Path $vs "VC\Tools\MSVC") -Directory | Where-Object { $_.Name -match '^14\.(3|4)\d\.' } | Sort-Object Name | Select-Object -Last 1
    if ($tools) { $MsvcVersion = ($tools.Name -split '\.')[0..1] -join '.' }
}
$vcvarsArg = if ($MsvcVersion) { "-vcvars_ver=$MsvcVersion" } else { "" }

# 3. Arquiteturas
$gencode = @()
$archs = $Arch -split ';'
foreach ($a in $archs) { $gencode += "-gencode=arch=compute_$a,code=sm_$a" }
$last = $archs[-1]
$gencode += "-gencode=arch=compute_$last,code=compute_$last"

Write-Host "CUDA: $Cuda | MSVC: $(if ($MsvcVersion) { $MsvcVersion } else { 'padrao' }) | sm: $Arch" -ForegroundColor Cyan
$tmp = Join-Path $env:TEMP "ghost_qcuda_build"
New-Item -ItemType Directory -Force $tmp | Out-Null
$bat = Join-Path $tmp "build.bat"
@"
@echo off
call "$vcvars" $vcvarsArg >nul 2>nul
cd /d "$tmp"
"$nvcc" -shared -O3 -Wno-deprecated-gpu-targets $($gencode -join ' ') -o "$out" "$src"
exit /b %ERRORLEVEL%
"@ | Set-Content -Encoding ascii $bat
cmd /c "`"$bat`""
if ($LASTEXITCODE -ne 0) { throw "nvcc falhou (codigo $LASTEXITCODE)" }
Remove-Item -ErrorAction SilentlyContinue ($out -replace '\.dll$', '.lib'), ($out -replace '\.dll$', '.exp')

# Copia para o build_win (testes do CTest rodam de la)
$bw = Join-Path $root "build_win"
if (Test-Path $bw) { Copy-Item $out $bw -Force }
Write-Host "OK: $out" -ForegroundColor Green

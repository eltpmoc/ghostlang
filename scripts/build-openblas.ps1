# Compila o OpenBLAS a partir do código-fonte oficial e instala em third_party/openblas.
#
# Por que do fonte: em 13/11/2026 o pacote mingw-w64-ucrt-x86_64-openblas do MSYS2 exigia o
# GCC 16 e o projeto estava no 15.2 (desde 19/11 o MSYS2 está no GCC 16). Continuamos compilando
# do fonte para ter a biblioteca estática, só C, com o OpenMP do GCC (o pacote traz o libomp do LLVM).
#
# Configuração: só C (NOFORTRAN=1), sem LAPACK (NO_LAPACK=1) e só precisão dupla
# (BUILD_DOUBLE=1), porque o runtime usa apenas cblas_dgemm; OpenMP do GCC; biblioteca
# estática; DYNAMIC_ARCH com DYNAMIC_LIST: o kernel certo é escolhido em tempo de execução
# entre AVX, AVX2, Zen e AVX-512. Com todas as famílias a compilação passa de 2 horas.
#
# Uso:  powershell -ExecutionPolicy Bypass -File scripts\build-openblas.ps1
# Depois: cmake -S . -B build (o CMake liga GHOST_USE_BLAS sozinho) e ninja.

param(
    [string]$Version = "0.3.30",
    [string]$Sha256  = "27342cff518646afb4c2b976d809102e368957974c250a25ccc965e53063c95d",
    [string]$Msys    = "C:\msys64",
    [int]$Jobs       = 8,
    # Famílias de CPU com kernel próprio (a de base, para CPUs antigas, entra sempre)
    [string]$DynamicList = "SANDYBRIDGE HASWELL ZEN SKYLAKEX"
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$dest = Join-Path $root "third_party\openblas"
$work = Join-Path $env:TEMP "ghost-openblas-build"
New-Item -ItemType Directory -Force $work | Out-Null

$tar = Join-Path $work "OpenBLAS-$Version.tar.gz"
if (-not (Test-Path $tar)) {
    $url = "https://github.com/OpenMathLib/OpenBLAS/releases/download/v$Version/OpenBLAS-$Version.tar.gz"
    Write-Host "Baixando $url"
    Invoke-WebRequest -Uri $url -OutFile $tar
}
$hash = (Get-FileHash $tar -Algorithm SHA256).Hash.ToLower()
if ($hash -ne $Sha256) { throw "SHA-256 nao confere: $hash (esperado $Sha256)" }

$bash = Join-Path $Msys "usr\bin\bash.exe"
$w = $work -replace '\\', '/'
& $bash -lc "export PATH=/ucrt64/bin:`$PATH; cd '$w' && rm -rf OpenBLAS-$Version && tar xzf OpenBLAS-$Version.tar.gz && cd OpenBLAS-$Version && make -j$Jobs NOFORTRAN=1 NO_LAPACK=1 BUILD_DOUBLE=1 USE_OPENMP=1 NO_SHARED=1 DYNAMIC_ARCH=1 DYNAMIC_LIST='$DynamicList' NUM_THREADS=64 CC=gcc HOSTCC=gcc libs"
if ($LASTEXITCODE -ne 0) { throw "make do OpenBLAS falhou" }

$src = Join-Path $work "OpenBLAS-$Version"
$lib = Get-ChildItem $src -Filter "libopenblas*.a" | Where-Object { $_.Name -notlike "*.dll.a" } | Select-Object -First 1
New-Item -ItemType Directory -Force (Join-Path $dest "lib") | Out-Null
Copy-Item $lib.FullName (Join-Path $dest "lib\libopenblas.a") -Force
Copy-Item (Join-Path $src "LICENSE") (Join-Path $dest "LICENSE") -Force
Set-Content -Encoding utf8 (Join-Path $dest "VERSAO.txt") "OpenBLAS $Version (sha256 $Sha256), NOFORTRAN=1 NO_LAPACK=1 BUILD_DOUBLE=1 USE_OPENMP=1 NO_SHARED=1 DYNAMIC_ARCH=1 DYNAMIC_LIST=$DynamicList"
Write-Host "OpenBLAS instalado em $dest"

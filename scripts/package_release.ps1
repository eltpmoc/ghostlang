# GhostLang - empacotador da release portátil para Windows x64.
#
# Gera dist/GhostLang-Portable-x64 (pasta), o .zip e o .sha256. O pacote roda numa máquina
# sem MSYS2: o GCC usado para compilar o runtime (MSYS2 UCRT64) vai embutido em
# bin/compiler, copiado arquivo a arquivo a partir da lista do pacman (pactree + pacman -Ql),
# sem documentação. O ghost procura <pasta do ghost.exe>/compiler/bin/g++.exe antes do PATH.
#
# Uso: powershell -ExecutionPolicy Bypass -File scripts\package_release.ps1 [-SemCompilador] [-SemZip]
# Teste do pacote: ctest -R portatil (roda o ghost do pacote com o PATH reduzido ao Windows).
param(
    [switch]$SemCompilador,
    [switch]$SemZip,
    [string]$Msys = "C:\msys64"
)
$ErrorActionPreference = "Stop"

$rootDir = (Resolve-Path "$PSScriptRoot/..").Path
$distDir = Join-Path $rootDir "dist/GhostLang-Portable-x64"
$zipOutput = Join-Path $rootDir "dist/GhostLang-Portable-x64.zip"
$binSrc = Join-Path $rootDir "build"

Write-Host "== Empacotador da release portatil - GhostLang ==" -ForegroundColor Cyan
foreach ($f in "ghost.exe", "libghost_runtime.a") {
    if (-not (Test-Path (Join-Path $binSrc $f))) { throw "Falta build/${f}: compile antes (ninja -C build)." }
}
if (Test-Path $distDir) { Remove-Item -Recurse -Force $distDir }
foreach ($d in "bin", "include", "libs", "docs", "docs/licencas", "examples") {
    New-Item -ItemType Directory -Force -Path (Join-Path $distDir $d) | Out-Null
}

Write-Host "[1/6] Binarios e runtime" -ForegroundColor Green
Copy-Item (Join-Path $binSrc "ghost.exe"), (Join-Path $binSrc "libghost_runtime.a") (Join-Path $distDir "bin") -Force
foreach ($extra in "ghost_node.exe", "ghost_swarm.exe", "ghost_api.exe", "ghost_ask.exe") {
    $p = Join-Path $binSrc $extra
    if (Test-Path $p) { Copy-Item $p (Join-Path $distDir "bin") -Force }
}

$gdbPriv = Join-Path $rootDir "third_party/gdb"
if (Test-Path (Join-Path $gdbPriv "bin/gdb.exe")) {
    # gdb privado (scripts/montar-gdb.ps1) para o ghost debug / --dap; o ghost procura bin/gdb/bin/gdb.exe
    Copy-Item -Recurse $gdbPriv (Join-Path $distDir "bin/gdb") -Force
}
Write-Host "[2/6] Cabecalhos C++ e bibliotecas .g" -ForegroundColor Green
Copy-Item (Join-Path $rootDir "include/*") (Join-Path $distDir "include") -Recurse -Force
Copy-Item (Join-Path $rootDir "libs/*.g") (Join-Path $distDir "libs") -Force

Write-Host "[3/6] Documentacao, exemplos e licencas" -ForegroundColor Green
foreach ($doc in "MANUAL.md", "SPEC.md", "NUCLEO.md", "GhostLang_Guia_Oficial_do_Programador.pdf", "GhostLang_Guia_Oficial_do_Programador.html") {
    $p = Join-Path $rootDir "docs/$doc"
    if (Test-Path $p) { Copy-Item $p (Join-Path $distDir "docs") -Force }
}
Copy-Item (Join-Path $rootDir "examples/*.g") (Join-Path $distDir "examples") -Force
Copy-Item (Join-Path $rootDir "LICENSE") (Join-Path $distDir "LICENSE.txt") -Force
$blasLic = Join-Path $rootDir "third_party/openblas/LICENSE"
if (Test-Path $blasLic) { Copy-Item $blasLic (Join-Path $distDir "docs/licencas/OpenBLAS-LICENSE.txt") -Force }

Write-Host "[4/6] Compilador embutido (GCC do MSYS2 UCRT64)" -ForegroundColor Green
if (-not $SemCompilador) {
    $bash = Join-Path $Msys "usr\bin\bash.exe"
    # gcc e, desde o GCC 16, o libgomp (OpenMP, usado por -fopenmp) vem num pacote separado.
    # pactree lista "nome=versao" para dependencias com versao fixa; o pacman -Ql quer so o nome.
    $lista = @()
    foreach ($raiz in "mingw-w64-ucrt-x86_64-gcc", "mingw-w64-ucrt-x86_64-libgomp") {
        $lista += (& $bash -lc "pactree -lu $raiz") | ForEach-Object { ($_ -split "[=<>]")[0] }
    }
    $pkgs = ($lista | Where-Object { $_ } | Sort-Object -Unique) -join " "
    if (-not $pkgs) { throw "pactree nao listou os pacotes do gcc" }
    $files = & $bash -lc "pacman -Qlq $pkgs" | Where-Object {
        $_ -like "/ucrt64/*" -and -not $_.EndsWith("/") -and $_ -notmatch "^/ucrt64/share/(doc|man|info|locale|gcc-[0-9.]+/python)/"
    }
    $compiler = Join-Path $distDir "bin/compiler"
    $n = 0
    foreach ($f in $files) {
        $rel = $f.Substring("/ucrt64/".Length)
        $src = Join-Path $Msys ("ucrt64/" + $rel)
        if (-not (Test-Path -LiteralPath $src -PathType Leaf)) { continue }
        $dst = Join-Path $compiler $rel
        $dir = Split-Path -Parent $dst
        if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
        [System.IO.File]::Copy($src, $dst, $true)
        $n++
    }
    Set-Content -Encoding utf8 (Join-Path $compiler "PACOTES.txt") ("GCC embutido a partir do MSYS2 UCRT64. Pacotes:`n" + ($pkgs -replace " ", "`n"))
    Write-Host "    $n arquivos copiados"
}

Write-Host "[5/6] Versao" -ForegroundColor Green
Copy-Item (Join-Path $rootDir "scripts/instalador/instalar.ps1"), (Join-Path $rootDir "scripts/instalador/desinstalar.ps1") $distDir -Force
Set-Content -Encoding ascii (Join-Path $distDir "instalar.bat") "@echo off`r`npowershell -NoProfile -ExecutionPolicy Bypass -File `"%~dp0instalar.ps1`" %*`r`npause"
Set-Content -Encoding ascii (Join-Path $distDir "iniciar_prompt.bat") "@echo off`r`nset PATH=%~dp0bin;%PATH%`r`necho GhostLang Portable: digite 'ghost' para o REPL ou 'ghost help'.`r`ncmd /k"
$ver = (& (Join-Path $distDir "bin/ghost.exe") version 2>&1 | Select-Object -First 1)
Set-Content -Encoding utf8 (Join-Path $distDir "VERSAO.txt") "$ver`nGerado em $(Get-Date -Format 'yyyy-MM-dd HH:mm')"

Write-Host "[6/6] ZIP e SHA-256" -ForegroundColor Green
if (-not $SemZip) {
    if (Test-Path $zipOutput) { Remove-Item -Force $zipOutput }
    Compress-Archive -Path "$distDir/*" -DestinationPath $zipOutput -CompressionLevel Optimal
    $sha256 = (Get-FileHash -Algorithm SHA256 $zipOutput).Hash.ToLower()
    Set-Content -Encoding ascii -Path "$distDir.sha256" -Value "$sha256  GhostLang-Portable-x64.zip"
    Write-Host "    ZIP: $zipOutput"
    Write-Host "    SHA256: $sha256"
}
Write-Host "Pacote pronto: $distDir" -ForegroundColor Cyan

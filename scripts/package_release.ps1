# GhostLang Release Packaging Script (PowerShell Multiplataforma)
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = (Resolve-Path "$scriptDir/..").Path
$distDir = "$rootDir/dist/GhostLang-Portable-x64"
$zipOutput = "$rootDir/dist/GhostLang-Portable-x64.zip"

Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "  Empacotador de Release Portatil - GhostLang (PS1)" -ForegroundColor Cyan
Write-Host "========================================================" -ForegroundColor Cyan

if (Test-Path $distDir) {
    Remove-Item -Recurse -Force $distDir
}

New-Item -ItemType Directory -Force -Path "$distDir/bin" | Out-Null
New-Item -ItemType Directory -Force -Path "$distDir/include" | Out-Null
New-Item -ItemType Directory -Force -Path "$distDir/libs" | Out-Null
New-Item -ItemType Directory -Force -Path "$distDir/docs" | Out-Null
New-Item -ItemType Directory -Force -Path "$distDir/examples" | Out-Null

Write-Host "[1/5] Copiando binarios executaveis..." -ForegroundColor Green
$binSrc = if (Test-Path "$rootDir/build_win/ghost.exe") { "$rootDir/build_win" } else { "$rootDir/build" }

Copy-Item "$binSrc/ghost.exe" "$distDir/bin/" -Force
if (Test-Path "$binSrc/ghost_node.exe") { Copy-Item "$binSrc/ghost_node.exe" "$distDir/bin/" -Force }
if (Test-Path "$binSrc/ghost_swarm.exe") { Copy-Item "$binSrc/ghost_swarm.exe" "$distDir/bin/" -Force }
if (Test-Path "$binSrc/libghost_runtime.a") { Copy-Item "$binSrc/libghost_runtime.a" "$distDir/bin/" -Force }

Write-Host "[2/5] Copiando headers da biblioteca C++..." -ForegroundColor Green
Get-ChildItem "$rootDir/include/*.hpp" | ForEach-Object { Copy-Item $_.FullName "$distDir/include/" -Force }

Write-Host "[3/5] Copiando bibliotecas GhostLang (.g)..." -ForegroundColor Green
Get-ChildItem "$rootDir/libs/*.g" | ForEach-Object { Copy-Item $_.FullName "$distDir/libs/" -Force }

Write-Host "[4/5] Copiando documentacao e manual..." -ForegroundColor Green
Copy-Item "$rootDir/docs/MANUAL.md" "$distDir/docs/" -Force
if (Test-Path "$rootDir/docs/SPEC.md") { Copy-Item "$rootDir/docs/SPEC.md" "$distDir/docs/" -Force }
if (Test-Path "$rootDir/docs/GhostLang_Guia_Oficial_do_Programador.pdf") {
    Copy-Item "$rootDir/docs/GhostLang_Guia_Oficial_do_Programador.pdf" "$distDir/docs/" -Force
}

Write-Host "[5/5] Gerando arquivo ZIP e Checksum SHA256..." -ForegroundColor Green
if (Test-Path $zipOutput) { Remove-Item -Force $zipOutput }
Compress-Archive -Path "$distDir/*" -DestinationPath $zipOutput -Force
$sha256 = (Get-FileHash -Algorithm SHA256 $zipOutput).Hash
Set-Content -Path "$distDir.sha256" -Value "$sha256  GhostLang-Portable-x64.zip"

Write-Host "Pacote criado com sucesso:" -ForegroundColor Yellow
Write-Host "  Pasta: $distDir"
Write-Host "  ZIP:   $zipOutput"
Write-Host "  SHA256: $sha256"
Write-Host "========================================================" -ForegroundColor Cyan

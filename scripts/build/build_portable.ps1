# build_portable.ps1
# Script para criar a distribuição portátil do GhostLang para Windows

# 1. Garante que estamos na raiz
Set-Location "$PSScriptRoot\..\.."

Write-Host "Iniciando build padrao do GhostLang..." -ForegroundColor Cyan
& "$PSScriptRoot\buildWin.ps1"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Falha ao compilar o GhostLang." -ForegroundColor Red
    exit $LASTEXITCODE
}

$dist = "GhostLang_Portable"
Write-Host "`nLimpando pasta $dist..." -ForegroundColor Yellow
if (Test-Path $dist) {
    Remove-Item -Recurse -Force $dist
}
New-Item -ItemType Directory -Path $dist | Out-Null

Write-Host "Copiando binários..." -ForegroundColor Cyan
Copy-Item "bin\ghost.exe", "bin\ghost_api.exe", "bin\ghost_node.exe", "bin\ghost_swarm.exe", "bin\ghost_ask.exe" -Destination $dist

Write-Host "Copiando bibliotecas padrão..." -ForegroundColor Cyan
Copy-Item -Recurse "include" -Destination $dist

Write-Host "`nBaixando compilador C++ portatil (WinLibs MinGW)..." -ForegroundColor Yellow
$mingwUrl = "https://github.com/brechtsanders/winlibs_mingw/releases/download/16.2.0posix-14.0.0-ucrt-r1/winlibs-x86_64-posix-seh-gcc-16.2.0-mingw-w64ucrt-14.0.0-r1.zip"
$zipPath = "$dist\mingw.zip"

Invoke-WebRequest -Uri $mingwUrl -OutFile $zipPath -UseBasicParsing

Write-Host "Extraindo compilador C++..." -ForegroundColor Yellow
Expand-Archive -Path $zipPath -DestinationPath $dist -Force
Remove-Item $zipPath

# A pasta padrao extraida pelo WinLibs chama-se "mingw64". Precisamos renomear para "compiler"
if (Test-Path "$dist\mingw64") {
    Rename-Item "$dist\mingw64" -NewName "compiler"
} else {
    Write-Host "Aviso: Pasta mingw64 nao encontrada na extracao!" -ForegroundColor Red
}

Write-Host "`nBaixando motor de IA nativo (llama-cli)..." -ForegroundColor Yellow
$releases = Invoke-RestMethod -Uri "https://api.github.com/repos/ggerganov/llama.cpp/releases"
$llamaUrl = ""
foreach ($r in $releases) {
    $asset = $r.assets | Where-Object { $_.name -match "-bin-win-vulkan-x64.zip" }
    if ($asset) {
        $llamaUrl = $asset.browser_download_url
        break
    }
}
$llamaZip = "$dist\llama.zip"
Invoke-WebRequest -Uri $llamaUrl -OutFile $llamaZip -UseBasicParsing
Expand-Archive -Path $llamaZip -DestinationPath "$dist\compiler" -Force
Remove-Item $llamaZip

Write-Host "`n==================================================" -ForegroundColor Green
Write-Host "SUCESSO! A pasta '$dist' esta pronta!" -ForegroundColor Green
Write-Host "Voce pode compactar essa pasta (.zip) e enviar para qualquer pessoa." -ForegroundColor Green
Write-Host "O GhostLang funcionara e compilara scripts nativamente sem dependencias externas." -ForegroundColor Green
Write-Host "==================================================" -ForegroundColor Green

# Instalador da GhostLang para Windows (vai na raiz do pacote portátil).
#
# Copia o pacote para a pasta de destino (padrão: %LOCALAPPDATA%\Programs\GhostLang), põe
# <destino>\bin no PATH do usuário (sem administrador) e cria um atalho no Menu Iniciar que
# abre um prompt com o ghost. Para remover: desinstalar.ps1 na pasta instalada.
#
# Uso: powershell -ExecutionPolicy Bypass -File instalar.ps1 [-Destino <pasta>] [-SemPath] [-SemAtalho]
param(
    [string]$Destino = (Join-Path $env:LOCALAPPDATA "Programs\GhostLang"),
    [switch]$SemPath,
    [switch]$SemAtalho
)
$ErrorActionPreference = "Stop"
$origem = $PSScriptRoot
if (-not (Test-Path (Join-Path $origem "bin\ghost.exe"))) { throw "Rode instalar.ps1 de dentro do pacote GhostLang (falta bin\ghost.exe)." }

$Destino = [System.IO.Path]::GetFullPath($Destino)
Write-Host "Instalando a GhostLang em $Destino"
if (Test-Path $Destino) {
    if (-not (Test-Path (Join-Path $Destino "bin\ghost.exe"))) { throw "$Destino existe e nao e uma instalacao da GhostLang." }
    Remove-Item -Recurse -Force $Destino
}
New-Item -ItemType Directory -Force -Path $Destino | Out-Null
Get-ChildItem -Force $origem | Where-Object { $_.Name -ne "instalar.ps1" } | ForEach-Object {
    Copy-Item -Recurse -Force $_.FullName (Join-Path $Destino $_.Name)
}

$bin = Join-Path $Destino "bin"
$registro = @{ destino = $Destino; path = $false; atalho = "" }

if (-not $SemPath) {
    $atual = [Environment]::GetEnvironmentVariable("Path", "User")
    $partes = @(); if ($atual) { $partes = $atual.Split(";") | Where-Object { $_ } }
    if ($partes -notcontains $bin) {
        [Environment]::SetEnvironmentVariable("Path", (($partes + $bin) -join ";"), "User")
        $registro.path = $true
        Write-Host "PATH do usuario: acrescentado $bin (abra um novo terminal)"
    }
}
if (-not $SemAtalho) {
    $menu = Join-Path ([Environment]::GetFolderPath("Programs")) "GhostLang.lnk"
    $shell = New-Object -ComObject WScript.Shell
    $lnk = $shell.CreateShortcut($menu)
    $lnk.TargetPath = Join-Path $Destino "iniciar_prompt.bat"
    $lnk.WorkingDirectory = $Destino
    $lnk.Description = "Prompt da GhostLang"
    $lnk.Save()
    $registro.atalho = $menu
    Write-Host "Atalho criado: $menu"
}
$registro | ConvertTo-Json | Set-Content -Encoding utf8 (Join-Path $Destino "instalacao.json")
& (Join-Path $bin "ghost.exe") version
Write-Host "Pronto. Para remover: powershell -ExecutionPolicy Bypass -File `"$(Join-Path $Destino 'desinstalar.ps1')`""

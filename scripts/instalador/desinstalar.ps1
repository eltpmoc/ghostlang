# Desinstalador da GhostLang: desfaz o que instalar.ps1 registrou em instalacao.json
# (pasta, entrada no PATH do usuário e atalho do Menu Iniciar).
# Uso: powershell -ExecutionPolicy Bypass -File <pasta instalada>\desinstalar.ps1
$ErrorActionPreference = "Stop"
$pasta = $PSScriptRoot
$info = Join-Path $pasta "instalacao.json"
if (-not (Test-Path $info) -or -not (Test-Path (Join-Path $pasta "bin\ghost.exe"))) {
    throw "Esta pasta nao parece uma instalacao da GhostLang (falta instalacao.json ou bin\ghost.exe)."
}
$reg = Get-Content -Raw $info | ConvertFrom-Json
$bin = Join-Path $pasta "bin"
if ($reg.path) {
    $atual = [Environment]::GetEnvironmentVariable("Path", "User")
    if ($atual) {
        $novo = ($atual.Split(";") | Where-Object { $_ -and $_ -ne $bin }) -join ";"
        [Environment]::SetEnvironmentVariable("Path", $novo, "User")
        Write-Host "PATH do usuario: removido $bin"
    }
}
if ($reg.atalho -and (Test-Path $reg.atalho)) { Remove-Item -Force $reg.atalho; Write-Host "Atalho removido" }
Set-Location (Split-Path -Parent $pasta)
Remove-Item -Recurse -Force $pasta
Write-Host "GhostLang removida de $pasta"

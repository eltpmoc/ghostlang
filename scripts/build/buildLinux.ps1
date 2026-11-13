Write-Host "=== GhostLang Linux Build via WSL ==="
Write-Host "Este script utiliza o Windows Subsystem for Linux (WSL) para compilar o projeto para Linux a partir do Windows."

# 1. Garante que estamos no diretório raiz do projeto
Set-Location "$PSScriptRoot\..\.."

# 2. Verifica se o WSL está disponível
try {
    $wslCheck = Get-Command wsl -ErrorAction SilentlyContinue
    if (-not $wslCheck) {
        Write-Host "Erro: WSL (Windows Subsystem for Linux) não encontrado." -ForegroundColor Red
        Write-Host "Você precisa ter o WSL instalado para compilar para Linux no Windows." -ForegroundColor Yellow
        Write-Host "Para instalar, abra o PowerShell como administrador e execute: wsl --install" -ForegroundColor Yellow
        exit
    }
} catch {
    Write-Host "Erro ao verificar WSL." -ForegroundColor Red
    exit
}

Write-Host "Iniciando ambiente Linux (WSL)..." -ForegroundColor Cyan

# 3. Executa o build.sh dentro do WSL
# O dos2unix garante que o build.sh não terá problemas com quebra de linha do Windows (\r\n) caso tenha sido editado
wsl bash -c "sudo apt-get install -y dos2unix cmake make g++ > /dev/null 2>&1; dos2unix scripts/build/buildLinux.sh 2>/dev/null; chmod +x scripts/build/buildLinux.sh; ./scripts/build/buildLinux.sh"

if ($LASTEXITCODE -eq 0) {
    Write-Host "Compilação para Linux finalizada! Os binários devem estar na pasta 'build_linux'." -ForegroundColor Green
} else {
    Write-Host "Ocorreu um erro durante a compilação via WSL." -ForegroundColor Red
}

Write-Host "FIM DO PROCESSO!!!"

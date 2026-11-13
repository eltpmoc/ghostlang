
Write-Host "Compilation Project Ghost"



Write-Host "# 1. Adiciona a pasta bin do MSYS2 UCRT64 e do CUDA ao PATH desta sessão do PowerShell"
Write-Host "# (Se o seu MSYS2 estiver instalado em outro local que não seja C:\msys64, ajuste o caminho abaixo)"
$env:PATH = "C:\msys64\ucrt64\bin;C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\bin;" + $env:PATH


Write-Host "# 2. Navega até a pasta do seu projeto"
Set-Location "$PSScriptRoot\..\.."

Write-Host "# 3. Cria a pasta build_win (e não exibe erro se ela já existir) e entra nela"
New-Item -ItemType Directory -Force -Path build_win
Set-Location build_win

Write-Host "# 4. Apaga tudo que está dentro da pasta build_win atual para zerar o cache"
Remove-Item -Recurse -Force *

Write-Host "# 5. Gera os arquivos de configuração de compilação via CMake"
cmake ..

Write-Host "# 6. Compila os executáveis (usando todos os núcleos disponíveis para ser mais rápido)"
cmake --build . -j $env:NUMBER_OF_PROCESSORS

Write-Host "# 7. Copia todos os arquivos .exe gerados na pasta build_win para o diretório bin"
Write-Host "# Isso garante que eles fiquem acessíveis de forma organizada"
Copy-Item -Path "*.exe" -Destination "..\bin\" -Force

Write-Host "# 8. Copia o runtime nativo para o bin: o ghost.exe liga os programas a ele"
Write-Host "# (sem ele, codigo que usa ghost/gate/state/IA falha no link)"
Copy-Item -Path "libghost_runtime.a" -Destination "..\bin\" -Force
Set-Location ..

Write-Host "# 9. Backend CUDA do simulador quantico (opcional: precisa de nvcc + Visual Studio C++)"
if (Test-Path "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA") {
    try { & "$PSScriptRoot\build_quantum_cuda.ps1" } catch { Write-Host "Aviso: backend CUDA nao compilado ($_). O simulador usara a CPU." -ForegroundColor Yellow }
}

Write-Host "FIM DO PROCESSO!!!"
# run_tests.ps1
# Script basico para testar a compilacao e execucao da GhostLang

$PSScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Definition
$compilerPath = Join-Path $PSScriptRoot "..\bin\ghost.exe"

if (-Not (Test-Path $compilerPath)) {
    Write-Host "Compilador (ghost.exe) não encontrado em $compilerPath. Por favor compile primeiro." -ForegroundColor Red
    exit 1
}

$testsDir = Join-Path $PSScriptRoot "ghost"
$testFiles = Get-ChildItem -Path $testsDir -Filter "*.g"

$failed = 0
$passed = 0

foreach ($file in $testFiles) {
    Write-Host "Testando: $($file.Name)... " -NoNewline
    
    $outFile = Join-Path $PSScriptRoot ($file.BaseName + ".exe")
    
    # Roda o compilador apontando para o arquivo de teste
    $process = Start-Process -FilePath $compilerPath -ArgumentList "$($file.FullName) -o $outFile" -NoNewWindow -Wait -PassThru -RedirectStandardError ".\error.log" -RedirectStandardOutput ".\out.log"
    
    # Se o nome do arquivo for error.g, esperamos que falhe (exit code 1)
    if ($file.Name -eq "error.g") {
        if ($process.ExitCode -eq 1) {
            Write-Host "PASSOU (Falhou conforme esperado)" -ForegroundColor Green
            $passed++
        } else {
            Write-Host "FALHOU (Deveria ter erro de compilação)" -ForegroundColor Red
            $failed++
        }
        $err = Get-Content ".\error.log"
        Write-Host "   -> Mensagem: $err" -ForegroundColor DarkGray
    } else {
        if ($process.ExitCode -eq 0) {
            Write-Host "PASSOU" -ForegroundColor Green
            $passed++
        } else {
            Write-Host "FALHOU" -ForegroundColor Red
            $failed++
            $err = Get-Content ".\error.log"
            $out = Get-Content ".\out.log"
            Write-Host "   -> Erro (stderr): $err" -ForegroundColor DarkRed
            Write-Host "   -> Out (stdout): $out" -ForegroundColor DarkRed
        }
    }
    
    if (Test-Path $outFile) { Remove-Item $outFile -ErrorAction Ignore }
}

if (Test-Path ".\error.log") { Remove-Item ".\error.log" }
if (Test-Path ".\out.log") { Remove-Item ".\out.log" }
if (Test-Path ".\_temp.cpp") { Remove-Item ".\_temp.cpp" }

Write-Host "-----------------------------"
Write-Host "Total: $($passed + $failed) | Passaram: $passed | Falharam: $failed"

if ($failed -gt 0) {
    exit 1
}
exit 0

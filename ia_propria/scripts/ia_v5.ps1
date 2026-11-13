$ErrorActionPreference = "Stop"

# Guardar diretório original
$OriginalLocation = Get-Location

# O script root é ia_propria/scripts. Vamos para ia_propria.
Set-Location -Path $PSScriptRoot\..

$env:GHOST_DEVICE = "auto"
$argsToPass = @()

# Analisar a arquitetura atual (runtime)
$runtimeConf = "..\build_native\ghost-runtime.conf"
$hasCuda = $false
if (Test-Path $runtimeConf) {
    $confLines = Get-Content $runtimeConf
    if ($confLines.Count -ge 3 -and $confLines[2] -eq "1") {
        $hasCuda = $true
    }
}

$i = 0
while ($i -lt $args.Count) {
    if ($args[$i] -eq "--device") {
        $i++
        if ($i -lt $args.Count) {
            $requestedDevice = $args[$i]
            if ($requestedDevice -eq "cuda" -and -not $hasCuda) {
                Write-Host "Aviso: Arquitetura atual nao possui runtime CUDA compilado. Fazendo fallback para CPU automaticamente." -ForegroundColor Yellow
                $env:GHOST_DEVICE = "cpu"
            } else {
                $env:GHOST_DEVICE = $requestedDevice
            }
        } else {
            Write-Error "Use --device auto|cpu|cuda"
            exit 1
        }
    } else {
        $arg = $args[$i]
        
        # Como mudamos o diretório para ia_propria, se o usuário passou o caminho
        # "ia_propria/..." estando na raiz, vamos ajustar o caminho relativo.
        if ($arg -match "^ia_propria[/\\](.*)") {
            $argsToPass += $Matches[1]
        } elseif (Test-Path "$OriginalLocation\$arg") {
            # Se for um arquivo relativo ao diretório que o usuário estava, resolvemos para absoluto
            $argsToPass += (Resolve-Path "$OriginalLocation\$arg").Path
        } else {
            $argsToPass += $arg
        }
    }
    $i++
}

if ($env:GHOST_DEVICE -notin @("auto", "cpu", "cuda", "gpu")) {
    Write-Error "Dispositivo invalido. Opcoes: auto, cpu, cuda, gpu"
    exit 1
}

if (-not $env:OMP_NUM_THREADS) {
    $env:OMP_NUM_THREADS = "4"
}

$exePath = ".\build\v5\ia"
if (Test-Path ".\build\v5\ia.exe") {
    $exePath = ".\build\v5\ia.exe"
}

if (-not (Test-Path $exePath)) {
    Write-Error "Executavel nao encontrado em $exePath. Execute antes scriptsmpilar_v5.ps1 (compilacao da V5 no Windows)."
    exit 1
}

& $exePath $argsToPass

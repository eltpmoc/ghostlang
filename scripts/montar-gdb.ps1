# Monta um gdb privado em third_party/gdb para o depurador da GhostLang (ghost debug --dap).
#
# Por que uma cópia: o pacote portátil precisa de um gdb que rode sem o MSYS2 instalado. O script
# copia o gdb.exe do MSYS2 e todas as DLLs de que ele depende (seguindo os imports com objdump)
# para third_party/gdb/bin; o empacotador leva essa pasta para bin/gdb.
# Histórico: em 13/11/2026 o gdb do MSYS2 exigia a libstdc++ do GCC 16 e o sistema estava no GCC 15,
# então o script também trocava a libstdc++/libgcc; desde 19/11/2026 o MSYS2 inteiro está no GCC 16.
#
# Pré-requisito: pacman -S mingw-w64-ucrt-x86_64-gdb
# Uso: powershell -ExecutionPolicy Bypass -File scripts\montar-gdb.ps1
param([string]$Msys = "C:\msys64")
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$dest = Join-Path $root "third_party\gdb"
$bin = Join-Path $dest "bin"
$ucrt = Join-Path $Msys "ucrt64\bin"
$objdump = Join-Path $ucrt "objdump.exe"
if (-not (Test-Path (Join-Path $ucrt "gdb.exe"))) { throw "Instale antes: pacman -S mingw-w64-ucrt-x86_64-gdb" }

if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }
New-Item -ItemType Directory -Force -Path $bin | Out-Null

# gdb.exe e a árvore de DLLs dele que vêm do ucrt64
$origem = ""
$fila = New-Object System.Collections.Queue
$vistos = @{}
$fila.Enqueue((Join-Path $ucrt "gdb.exe"))
while ($fila.Count -gt 0) {
    $atual = $fila.Dequeue()
    Copy-Item $atual $bin -Force
    foreach ($linha in (& $objdump -p $atual | Select-String "DLL Name: (\S+)")) {
        $dll = $linha.Matches[0].Groups[1].Value
        $p = Join-Path $ucrt $dll
        if ((Test-Path $p) -and -not $vistos.ContainsKey($dll)) { $vistos[$dll] = $true; $fila.Enqueue($p) }
    }
}

# scripts de dados do gdb (python, syscalls) e a biblioteca padrão do Python embutido
$share = Join-Path $Msys "ucrt64\share\gdb"
if (Test-Path $share) { Copy-Item -Recurse $share (Join-Path $dest "share\gdb") -Force }
Set-Content -Encoding utf8 (Join-Path $dest "ORIGEM.txt") "gdb do MSYS2 (mingw-w64-ucrt-x86_64-gdb) com as DLLs do ucrt64. Montado por scripts/montar-gdb.ps1."
& (Join-Path $bin "gdb.exe") --version | Select-Object -First 1

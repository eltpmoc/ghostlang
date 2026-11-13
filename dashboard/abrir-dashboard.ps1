# Abre o dashboard de status da GhostLang em um servidor local (somente localhost).
# Uso: powershell -ExecutionPolicy Bypass -File dashboard\abrir-dashboard.ps1 [-Porta 8777] [-SemNavegador]
param(
    [int]$Porta = 8777,
    [switch]$SemNavegador
)

$ErrorActionPreference = 'Stop'
$raiz = Split-Path -Parent $MyInvocation.MyCommand.Path
$prefixo = "http://localhost:$Porta/"

$tipos = @{
    '.html' = 'text/html; charset=utf-8'
    '.json' = 'application/json; charset=utf-8'
    '.js'   = 'text/javascript; charset=utf-8'
    '.css'  = 'text/css; charset=utf-8'
    '.svg'  = 'image/svg+xml'
    '.png'  = 'image/png'
    '.ico'  = 'image/x-icon'
}

$listener = New-Object System.Net.HttpListener
$listener.Prefixes.Add($prefixo)
try {
    $listener.Start()
} catch {
    Write-Host "Nao foi possivel abrir a porta $Porta. Tente: -Porta 8780" -ForegroundColor Red
    exit 1
}

Write-Host "GhostLang Status em $prefixo  (Ctrl+C para encerrar)" -ForegroundColor Cyan
if (-not $SemNavegador) { Start-Process $prefixo }

try {
    while ($listener.IsListening) {
        $ctx = $listener.GetContext()
        $res = $ctx.Response
        try {
            $rel = [Uri]::UnescapeDataString($ctx.Request.Url.AbsolutePath.TrimStart('/'))
            if ([string]::IsNullOrEmpty($rel)) { $rel = 'index.html' }
            $caminho = [IO.Path]::GetFullPath((Join-Path $raiz $rel))
            # Bloqueia acesso fora da pasta do dashboard
            if (-not $caminho.StartsWith($raiz, [StringComparison]::OrdinalIgnoreCase) -or -not (Test-Path $caminho -PathType Leaf)) {
                $res.StatusCode = 404
                $bytes = [Text.Encoding]::UTF8.GetBytes('404')
            } else {
                $ext = [IO.Path]::GetExtension($caminho).ToLower()
                $res.ContentType = if ($tipos.ContainsKey($ext)) { $tipos[$ext] } else { 'application/octet-stream' }
                $res.Headers.Add('Cache-Control', 'no-store')
                $bytes = [IO.File]::ReadAllBytes($caminho)
            }
            $res.ContentLength64 = $bytes.Length
            $res.OutputStream.Write($bytes, 0, $bytes.Length)
        } catch {
            $res.StatusCode = 500
        } finally {
            $res.OutputStream.Close()
        }
    }
} finally {
    $listener.Stop()
}

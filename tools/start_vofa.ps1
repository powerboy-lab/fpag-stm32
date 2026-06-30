param(
    [string]$ExePath = "D:\Program Files\Gutega\VOFA+\x64\vofa+.exe"
)

if (-not (Test-Path -LiteralPath $ExePath)) {
    Write-Error "VOFA+ not found: $ExePath"
    exit 1
}

Start-Process -FilePath $ExePath

# Display7 derleme — kaynak degistiyse mikro++, Sync, idf.py build
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "scripts\Bump-IfChanged.ps1")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
. C:\Espressif\frameworks\esp-idf-v5.5.2\export.ps1
idf.py build
exit $LASTEXITCODE

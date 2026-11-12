<#
.SYNOPSIS
  Kaynak (main/, components/hexnet) degistiyse mikro++ ve Sync; aksi halde sadece Sync.
#>
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$wsTools = Join-Path (Split-Path -Parent (Split-Path -Parent $Root)) "tools\hexnet-version\HexnetVersion.psm1"
if (-not (Test-Path -LiteralPath $wsTools -PathType Leaf)) {
    Write-Warning "Hexnet version module not found at '$wsTools'; skipping automatic version bump."
    exit 0
}
Import-Module $wsTools -Force
Invoke-HexnetBumpIfSourceChanged -Root $Root -Kind display -Notes "auto: source changed" | Out-Null

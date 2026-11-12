# Sync VERSION.json -> VERSION, hexnet_version.h, CMakeLists.txt
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$wsTools = Join-Path (Split-Path -Parent (Split-Path -Parent $Root)) "tools\hexnet-version\HexnetVersion.psm1"
Import-Module $wsTools -Force
Sync-HexnetFirmwareFiles -Root $Root -Kind display

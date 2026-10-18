param([switch]$ClientOnly)
$ErrorActionPreference = 'Stop'
$clientRoot = 'C:\Jeux\WoW335a'
$serverRoot = 'C:\TC\bin'
$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$processNames = if ($ClientOnly) { @('WowClient64') } else { @('WowClient64', 'worldserver') }
if (Get-Process -Name $processNames -ErrorAction SilentlyContinue) {
    throw 'Ferme WowClient64 et worldserver avant de copier les nouveaux executables.'
}
$items = @(
    @{ Source = (Join-Path $repoRoot 'bin\Win\WowClient64.exe'); Target = (Join-Path $clientRoot 'WowClient64.exe') },
    @{ Source = (Join-Path $repoRoot 'bin\Win\XvidDecoder.dll'); Target = (Join-Path $clientRoot 'XvidDecoder.dll') },
    @{ Source = 'C:\TC\build\bin\Release\worldserver.exe'; Target = (Join-Path $serverRoot 'worldserver.exe') },
    @{ Source = (Join-Path $PSScriptRoot 'addon\GMBlueprint\GMBlueprint.toc'); Target = (Join-Path $clientRoot 'Interface\AddOns\GMBlueprint\GMBlueprint.toc') },
    @{ Source = (Join-Path $PSScriptRoot 'addon\GMBlueprint\GMBlueprint.lua'); Target = (Join-Path $clientRoot 'Interface\AddOns\GMBlueprint\GMBlueprint.lua') },
    @{ Source = (Join-Path $PSScriptRoot 'addon\GMBlueprint\Locales.lua'); Target = (Join-Path $clientRoot 'Interface\AddOns\GMBlueprint\Locales.lua') },
    @{ Source = (Join-Path $PSScriptRoot 'addon\GMBlueprint\CatalogData.lua'); Target = (Join-Path $clientRoot 'Interface\AddOns\GMBlueprint\CatalogData.lua') },
    @{ Source = (Join-Path $PSScriptRoot 'addon\GMBlueprint\Catalog.lua'); Target = (Join-Path $clientRoot 'Interface\AddOns\GMBlueprint\Catalog.lua') }
)
if ($ClientOnly) {
    $items = @($items | Where-Object { $_.Target -ne (Join-Path $serverRoot 'worldserver.exe') })
}
foreach ($item in $items) {
    if (-not (Test-Path -LiteralPath $item.Source -PathType Leaf)) { throw "Fichier absent : $($item.Source)" }
}
if (-not (Test-Path -LiteralPath $clientRoot -PathType Container)) { throw 'Dossier du jeu absent.' }
if (-not (Test-Path -LiteralPath $serverRoot -PathType Container)) { throw 'Dossier du serveur absent.' }
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$clientBackup = Join-Path $clientRoot "BlueprintBackup-$stamp"
$serverBackup = Join-Path 'C:\TC' "blueprint-bin-backup-$stamp"
New-Item -ItemType Directory -Path $clientBackup | Out-Null
if (-not $ClientOnly) { New-Item -ItemType Directory -Path $serverBackup | Out-Null }
# Back up every existing target before replacing any of them.
foreach ($item in $items) {
    if (Test-Path -LiteralPath $item.Target) {
        $backupRoot = if ($item.Target.StartsWith($clientRoot, [StringComparison]::OrdinalIgnoreCase)) { $clientBackup } else { $serverBackup }
        $item.Backup = Join-Path $backupRoot ([System.IO.Path]::GetFileName($item.Target))
        Copy-Item -LiteralPath $item.Target -Destination $item.Backup
    }
}
foreach ($item in $items) {
    $targetDir = Split-Path -Parent $item.Target
    New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
    Copy-Item -LiteralPath $item.Source -Destination $item.Target -Force
    $item.Hash = (Get-FileHash -LiteralPath $item.Target).Hash
    if ($item.Hash -ne (Get-FileHash -LiteralPath $item.Source).Hash) { throw "Verification echouee : $($item.Target)" }
}
$manifest = if ($ClientOnly) { 'build\x64\GMBlueprint-client-install.json' } else { 'build\x64\GMBlueprint-install.json' }
$items | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $repoRoot $manifest) -Encoding UTF8
Write-Output "Sauvegarde client : $clientBackup"
if (-not $ClientOnly) { Write-Output "Sauvegarde serveur : $serverBackup" }
Write-Output 'Installation terminee ; copies verifiees par SHA-256. Aucun processus lance ou arrete.'

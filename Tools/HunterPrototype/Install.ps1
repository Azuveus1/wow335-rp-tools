$ErrorActionPreference='Stop'
if (Get-Process worldserver,WowClient64 -ErrorAction SilentlyContinue) {throw 'Fermer WoW et arreter worldserver proprement avant installation.'}
$source='C:\TC\build\bin\Release\worldserver.exe'
$target='C:\TC\bin\worldserver.exe'
$clientRoot='C:\Jeux\WoW335a'
$addonRoot=Join-Path $clientRoot 'Interface\AddOns\HunterContracts'
if (-not (Test-Path -LiteralPath $clientRoot -PathType Container)) {throw 'Dossier client introuvable.'}
$items=@(@{Source=$source;Target=$target;Backup='worldserver.exe'})
foreach ($name in @('HunterContracts.toc','HunterContracts.lua')) {
 $items+=@{Source=(Join-Path $PSScriptRoot ('addon\HunterContracts\'+$name));Target=(Join-Path $addonRoot $name);Backup=('HunterContracts\'+$name)}
}
foreach ($item in $items) {if (-not (Test-Path -LiteralPath $item.Source -PathType Leaf)) {throw ('Fichier source absent : '+$item.Source)}}
$backup=Join-Path 'C:\TC' ('hunter-prototype-bin-backup-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $backup | Out-Null
foreach ($item in $items) {
 if (Test-Path -LiteralPath $item.Target -PathType Leaf) {
  $saved=Join-Path $backup $item.Backup
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $saved) | Out-Null
  Copy-Item -LiteralPath $item.Target -Destination $saved
 }
}
foreach ($item in $items) {
 New-Item -ItemType Directory -Force -Path (Split-Path -Parent $item.Target) | Out-Null
 Copy-Item -LiteralPath $item.Source -Destination $item.Target -Force
 if ((Get-FileHash -LiteralPath $item.Source).Hash -ne (Get-FileHash -LiteralPath $item.Target).Hash) {throw ('Verification echouee : '+$item.Target)}
}
Write-Output "Sauvegarde : $backup"
Write-Output "Interface installee : $addonRoot"
Write-Output 'Serveur et interface du panneau installes ; copies verifiees par SHA-256. Processus laisses arretes.'

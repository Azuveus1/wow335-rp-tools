param([switch]$ClientOnly)
$ErrorActionPreference='Stop'
$processNames=if ($ClientOnly) {@('WowClient64')} else {@('WowClient64','worldserver')}
if (Get-Process -Name $processNames -ErrorAction SilentlyContinue) {throw 'Fermer les programmes concernes avant installation.'}
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$items=@(
 @{Source=(Join-Path $root 'bin\Win\WowClient64.exe');Target='C:\Jeux\WoW335a\WowClient64.exe';Backup='WowClient64.exe'},
 @{Source='C:\TC\build\bin\Release\worldserver.exe';Target='C:\TC\bin\worldserver.exe';Backup='worldserver.exe'}
)
if ($ClientOnly) {$items=@($items | Where-Object {$_.Backup -eq 'WowClient64.exe'})}
foreach ($item in $items) {if (-not (Test-Path -LiteralPath $item.Source -PathType Leaf)) {throw ('Fichier source absent : '+$item.Source)}}
$backup=Join-Path 'C:\TC' ('rp-names-bin-backup-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $backup | Out-Null
foreach ($item in $items) {Copy-Item -LiteralPath $item.Target -Destination (Join-Path $backup $item.Backup)}
foreach ($item in $items) {
 Copy-Item -LiteralPath $item.Source -Destination $item.Target -Force
 if ((Get-FileHash -LiteralPath $item.Source).Hash -ne (Get-FileHash -LiteralPath $item.Target).Hash) {throw ('Verification echouee : '+$item.Target)}
}
Write-Output "Sauvegarde : $backup"
if ($ClientOnly) {Write-Output 'Client installe, SHA-256 verifie. Serveur laisse en cours execution.'}
else {Write-Output 'Client et serveur installes, SHA-256 verifies. Processus laisses arretes.'}

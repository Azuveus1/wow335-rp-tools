$ErrorActionPreference='Stop'
if (Get-Process -Name WowClient64,worldserver -ErrorAction SilentlyContinue) {throw 'Fermer WoW et worldserver avant installation.'}
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$files=@(
 @{Source=(Join-Path $taskRoot 'bin\Win\WowClient64.exe');Target='C:\Jeux\WoW335a\WowClient64.exe';Name='WowClient64.exe'},
 @{Source='C:\TC\build\bin\Release\worldserver.exe';Target='C:\TC\bin\worldserver.exe';Name='worldserver.exe'},
 @{Source=(Join-Path $PSScriptRoot 'CivilRegistry.toc');Target='C:\Jeux\WoW335a\Interface\AddOns\CivilRegistry\CivilRegistry.toc';Name='CivilRegistry.toc'},
 @{Source=(Join-Path $PSScriptRoot 'CivilRegistry.lua');Target='C:\Jeux\WoW335a\Interface\AddOns\CivilRegistry\CivilRegistry.lua';Name='CivilRegistry.lua'},
 @{Source=(Join-Path $PSScriptRoot 'WeddingSlot.lua');Target='C:\Jeux\WoW335a\Interface\AddOns\CivilRegistry\WeddingSlot.lua';Name='WeddingSlot.lua'}
)
foreach ($file in $files) {if (-not (Test-Path -LiteralPath $file.Source -PathType Leaf)) {throw ('Source manquante : '+$file.Source)}}
$backup=Join-Path 'C:\TC' ('civil-registry-backup-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $backup | Out-Null
foreach ($file in $files) {if (Test-Path -LiteralPath $file.Target -PathType Leaf) {Copy-Item -LiteralPath $file.Target -Destination (Join-Path $backup $file.Name)}}
$manifest=@()
foreach ($file in $files) {
 New-Item -ItemType Directory -Force -Path (Split-Path -Parent $file.Target) | Out-Null
 Copy-Item -LiteralPath $file.Source -Destination $file.Target -Force
 $hash=(Get-FileHash -LiteralPath $file.Source).Hash
 if ($hash -ne (Get-FileHash -LiteralPath $file.Target).Hash) {throw ('Verification SHA-256 echouee : '+$file.Target)}
 $manifest+=@{Path=$file.Target;SHA256=$hash}
}
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $backup 'installed-manifest.json')
Write-Output "Client, serveur et addon installes. SHA-256 verifies. Sauvegarde : $backup"
Write-Output 'Processus laisses arretes. Relancer worldserver puis WoW pour tester.'

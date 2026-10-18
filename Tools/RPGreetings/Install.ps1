$ErrorActionPreference='Stop'
if (Get-Process worldserver -ErrorAction SilentlyContinue) {throw 'Arreter worldserver proprement avant installation.'}
$source='C:\TC\build\bin\Release\worldserver.exe'
$target='C:\TC\bin\worldserver.exe'
if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {throw 'Serveur compile introuvable.'}
$backup=Join-Path 'C:\TC' ('rp-greetings-bin-backup-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $backup | Out-Null
Copy-Item -LiteralPath $target -Destination (Join-Path $backup 'worldserver.exe')
Copy-Item -LiteralPath $source -Destination $target -Force
if ((Get-FileHash -LiteralPath $source).Hash -ne (Get-FileHash -LiteralPath $target).Hash) {throw 'Verification SHA-256 echouee.'}
Write-Output "Sauvegarde : $backup"
Write-Output 'Worldserver installe et verifie ; serveur laisse arrete. Client non remplace.'

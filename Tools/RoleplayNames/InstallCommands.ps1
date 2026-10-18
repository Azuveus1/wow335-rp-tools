$ErrorActionPreference='Stop'
if (Get-Process -Name worldserver -ErrorAction SilentlyContinue) {throw 'Arreter worldserver proprement avant installation.'}
$source='C:\TC\build\bin\Release\worldserver.exe'
$target='C:\TC\bin\worldserver.exe'
$backup=Join-Path 'C:\TC' ('rp-command-backup-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $backup | Out-Null
Copy-Item -LiteralPath $target -Destination (Join-Path $backup 'worldserver.exe')
Copy-Item -LiteralPath $source -Destination $target -Force
if ((Get-FileHash -LiteralPath $source).Hash -ne (Get-FileHash -LiteralPath $target).Hash) {throw 'Verification SHA-256 echouee'}
Write-Output "Serveur installe, SHA-256 verifie. Sauvegarde : $backup"
Write-Output 'Worldserver laisse arrete ; le client reste inchange.'

$ErrorActionPreference = 'Stop'
if (Get-Process WowClient64,worldserver -ErrorAction SilentlyContinue) { throw 'Fermer WoW et worldserver avant installation.' }
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$client = 'C:\Jeux\WoW335a'
$server = 'C:\TC\bin'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$clientBackup = Join-Path $client "TransportBackup-$stamp"
$serverBackup = Join-Path 'C:\TC' "transport-bin-backup-$stamp"
$items = @(
    @{Source=(Join-Path $repo 'bin\Win\WowClient64.exe');Target=(Join-Path $client 'WowClient64.exe');Backup=(Join-Path $clientBackup 'WowClient64.exe')},
    @{Source='C:\TC\build\bin\Release\worldserver.exe';Target=(Join-Path $server 'worldserver.exe');Backup=(Join-Path $serverBackup 'worldserver.exe')}
)
foreach ($file in Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'addon\GMTransport') -File) {
    $items += @{Source=$file.FullName;Target=(Join-Path $client "Interface\AddOns\GMTransport\$($file.Name)");Backup=(Join-Path $clientBackup "addon\$($file.Name)")}
}
foreach ($item in $items) {
    if (!(Test-Path -LiteralPath $item.Source -PathType Leaf)) { throw "Source absente : $($item.Source)" }
}
foreach ($item in $items) {
    if (Test-Path -LiteralPath $item.Target) {
        New-Item -ItemType Directory -Path (Split-Path -Parent $item.Backup) -Force | Out-Null
        Copy-Item -LiteralPath $item.Target -Destination $item.Backup
    }
}
foreach ($item in $items) {
    New-Item -ItemType Directory -Path (Split-Path -Parent $item.Target) -Force | Out-Null
    Copy-Item -LiteralPath $item.Source -Destination $item.Target -Force
    $item.Hash = (Get-FileHash -LiteralPath $item.Target).Hash
    if ($item.Hash -ne (Get-FileHash -LiteralPath $item.Source).Hash) { throw "Verification echouee : $($item.Target)" }
}
$items | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $repo 'build\transport-install.json') -Encoding UTF8
Write-Output "Sauvegarde client : $clientBackup"
Write-Output "Sauvegarde serveur : $serverBackup"
Write-Output 'Installation verifiee par SHA-256. Aucun processus lance.'

param([switch]$Disable)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root 'dist\TrayFolders.exe'
$runKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$valueName = 'TrayFolders'
if ($Disable) {
    if (Test-Path -LiteralPath $runKey) {
        Remove-ItemProperty -LiteralPath $runKey -Name $valueName -ErrorAction SilentlyContinue
    }
    '已关闭托盘文件夹的登录自启动。'
    return
}
if (!(Test-Path -LiteralPath $exe)) { throw 'Install the application to dist before enabling autostart.' }
$command = '"' + $exe + '" --silent'
New-Item -Path $runKey -Force | Out-Null
New-ItemProperty -LiteralPath $runKey -Name $valueName -PropertyType String -Value $command -Force | Out-Null
# Reset only this app's previous disabled state, if Windows has recorded one.
$approvalKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run'
if (Test-Path -LiteralPath $approvalKey) {
    $key = Get-Item -LiteralPath $approvalKey
    if ($key.GetValueNames() -contains $valueName) {
        Remove-ItemProperty -LiteralPath $approvalKey -Name $valueName
    }
}
[pscustomobject]@{Enabled=$true;Scope='Current user logon';Command=(Get-ItemProperty -LiteralPath $runKey -Name $valueName).$valueName}

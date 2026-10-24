param([string]$InstallDirectory = '', [switch]$NoRestart)
. (Join-Path $PSScriptRoot 'Build.Common.ps1')
if (!$InstallDirectory) { $InstallDirectory = Join-Path $projectRoot 'dist' }
$InstallDirectory = [IO.Path]::GetFullPath($InstallDirectory)
if ($InstallDirectory.TrimEnd('\') -eq $binDir.TrimEnd('\')) { throw 'Install directory cannot be the build output directory.' }
$app = Join-Path $binDir 'TrayFolders.exe'
$appConfig = Join-Path $binDir 'TrayFolders.exe.config'
if (!(Test-Path -LiteralPath $appConfig)) { throw 'Build the application DPI configuration first.' }
$native = Join-Path $binDir $project.NativeLibrary
$resolver = Join-Path $binDir 'TrayFolders.Symbols.exe'
if (!(Test-Path -LiteralPath $resolver)) { throw 'Build the symbol resolver first.' }
if (!(Test-Path -LiteralPath $app) -or !(Test-Path -LiteralPath $native)) { throw 'Run a full build first.' }
if ((Get-Item -LiteralPath $app).VersionInfo.FileVersion -ne $project.Version) { throw 'Application build is stale.' }
New-Item -ItemType Directory -Path $InstallDirectory -Force | Out-Null
$installedExe = Join-Path $InstallDirectory 'TrayFolders.exe'
$installedNative = Join-Path $InstallDirectory $project.NativeLibrary
$nativeExists = Test-Path -LiteralPath $installedNative
if ($nativeExists -and (Get-FileHash -LiteralPath $native).Hash -ne (Get-FileHash -LiteralPath $installedNative).Hash) {
    throw 'Native library contents changed under the same filename. Increment NativeLibrary in project.psd1 and rebuild before updating.'
}
$running = @(Get-CimInstance Win32_Process -Filter "Name='TrayFolders.exe'" | Where-Object { $_.ExecutablePath -eq $installedExe })
if ($running.Count) {
    $stop = Start-Process -FilePath $installedExe -ArgumentList '--stop' -WindowStyle Hidden -PassThru -Wait
    $result = [IO.File]::ReadAllText((Join-Path $InstallDirectory 'command-result.txt')).Trim()
    if ($stop.ExitCode -ne 0 -or $result -ne '0') { throw "Could not stop the existing core: $result" }
    foreach ($item in $running) {
        $process = Get-Process -Id $item.ProcessId -ErrorAction Stop
        if ($process.Path -ne $installedExe) { throw 'Process identity changed; not stopping it.' }
        Stop-Process -Id $process.Id
        $process.WaitForExit()
    }
}
$backup = Join-Path $buildDir ('backups\' + [Guid]::NewGuid().ToString('N'))
$installedResolver = Join-Path $InstallDirectory 'TrayFolders.Symbols.exe'
foreach ($worker in @(Get-CimInstance Win32_Process -Filter "Name='TrayFolders.Symbols.exe'" | Where-Object { $_.ExecutablePath -eq $installedResolver })) {
    $process = Get-Process -Id $worker.ProcessId -ErrorAction Stop
    if ($process.Path -ne $installedResolver) { throw 'Resolver process identity changed.' }
    Stop-Process -Id $process.Id
    $process.WaitForExit()
}
New-Item -ItemType Directory -Path $backup -Force | Out-Null
$before = @{}
foreach ($name in @('TrayFolders.exe','TrayFolders.exe.config','TrayFolders.Symbols.exe','settings.ini','assignments.ini')) {
    $file = Join-Path $InstallDirectory $name
    if (Test-Path -LiteralPath $file) {
        Copy-Item -LiteralPath $file -Destination $backup
        if ($name -in @('settings.ini','assignments.ini')) { $before[$name] = (Get-FileHash -LiteralPath $file).Hash }
    }
}
Copy-Item -LiteralPath $app -Destination $installedExe -Force
Copy-Item -LiteralPath $appConfig -Destination ($installedExe + '.config') -Force
Copy-Item -LiteralPath $resolver -Destination (Join-Path $InstallDirectory 'TrayFolders.Symbols.exe') -Force
if (!$nativeExists) { Copy-Item -LiteralPath $native -Destination $installedNative }
foreach ($name in @('settings.ini','assignments.ini')) {
    $file = Join-Path $InstallDirectory $name
    if (!(Test-Path -LiteralPath $file)) { Copy-Item -LiteralPath (Join-Path $projectRoot ('config\defaults\'+$name)) -Destination $file }
    if ($before.ContainsKey($name) -and (Get-FileHash -LiteralPath $file).Hash -ne $before[$name]) { throw "Configuration unexpectedly changed: $name" }
}
$restarted = $null
if ($running.Count -and !$NoRestart) {
    $restarted = (Start-Process -FilePath $installedExe -WorkingDirectory $InstallDirectory -WindowStyle Hidden -PassThru).Id
}
[pscustomobject]@{InstallDirectory=$InstallDirectory;Version=$project.Version;ConfigurationPreserved=$true;RestartedProcessId=$restarted;Backup=$backup}

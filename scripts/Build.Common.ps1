# Shared paths and compiler invocation. Requires PowerShell 7.
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Import-PowerShellDataFile (Join-Path $projectRoot 'project.psd1')
$buildDir = Join-Path $projectRoot 'build'
$binDir = Join-Path $buildDir 'bin'
$csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if ($project.NativeLibrary -notmatch '^TrayFolders\.Native\.v[0-9]+\.dll$') { throw 'Invalid NativeLibrary in project.psd1.' }
$null = [version]::Parse($project.Version)

function Invoke-Compiler([string]$Executable, [string[]]$Arguments) {
    $start = [Diagnostics.ProcessStartInfo]::new($Executable)
    $start.WorkingDirectory = $projectRoot
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    foreach ($argument in $Arguments) { $start.ArgumentList.Add($argument) }
    $process = [Diagnostics.Process]::Start($start)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $output = $stdout.GetAwaiter().GetResult() + $stderr.GetAwaiter().GetResult()
    if ($output) { Write-Output $output }
    if ($process.ExitCode -ne 0) { throw "Compiler failed ($($process.ExitCode)): $Executable" }
}

function Get-FrameworkReferences {
    @('/reference:System.dll','/reference:System.Core.dll','/reference:System.Drawing.dll','/reference:System.Windows.Forms.dll')
}

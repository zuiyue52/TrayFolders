param([string]$OutputDirectory = '', [switch]$IncludeReleaseChecks)
. (Join-Path $PSScriptRoot 'Build.Common.ps1')
& (Join-Path $PSScriptRoot 'Build.ps1') -AppOnly
$testsDir = Join-Path $buildDir 'tests'
if (!$OutputDirectory) { $OutputDirectory = Join-Path $buildDir ('test-results\' + [Guid]::NewGuid().ToString('N')) }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $testsDir,$OutputDirectory -Force | Out-Null
$app = Join-Path $binDir 'TrayFolders.exe'
$testExe = Join-Path $testsDir 'TrayFolders.Tests.exe'
$sources = @(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'tests') -Recurse -Filter '*.cs' | Sort-Object FullName | ForEach-Object FullName)
Invoke-Compiler $csc (@('/nologo','/target:winexe','/platform:x64','/optimize+',('/win32manifest:'+(Join-Path $projectRoot 'src\app\app.manifest')),('/reference:'+$app),('/out:'+$testExe)) + (Get-FrameworkReferences) + $sources)
Copy-Item -LiteralPath (Join-Path $projectRoot 'src\app\app.config') -Destination ($testExe + '.config') -Force
Copy-Item -LiteralPath $app -Destination (Join-Path $testsDir 'TrayFolders.exe') -Force
$start = [Diagnostics.ProcessStartInfo]::new($testExe)
$start.UseShellExecute = $false
$start.CreateNoWindow = $true
$start.ArgumentList.Add($OutputDirectory)
$process = [Diagnostics.Process]::Start($start)
if (!$process.WaitForExit(180000)) { $process.Kill(); throw 'Isolated tests timed out.' }
$result = Join-Path $OutputDirectory 'result.txt'
if (Test-Path -LiteralPath $result) { Get-Content -LiteralPath $result }
if ($process.ExitCode -ne 0) { throw "Tests failed ($($process.ExitCode)). Results: $OutputDirectory" }
"Test artifacts: $OutputDirectory"
if ($IncludeReleaseChecks) { & (Join-Path $projectRoot 'tests\ReleaseTests.ps1') }

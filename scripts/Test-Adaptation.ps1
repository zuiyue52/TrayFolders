param(
    [Parameter(Mandatory=$true)][string]$Module,
    [Parameter(Mandatory=$true)][string]$CacheDirectory,
    [string]$CompilerRoot = $env:LLVM_MINGW_ROOT,
    [switch]$AllowDownload
)
. (Join-Path $PSScriptRoot 'Build.Common.ps1')
if (!$CompilerRoot) { throw 'Provide -CompilerRoot for the isolated native tests.' }
$output = Join-Path $buildDir ('adaptation-tests\' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output -Force | Out-Null
$profile = Join-Path $output 'compatibility.bin'
$resolver = Join-Path $binDir 'TrayFolders.Symbols.exe'
$arguments = @($Module,[IO.Path]::GetFullPath($CacheDirectory),$profile)
if (!$AllowDownload) { $arguments += '--offline' }
Invoke-Compiler $resolver $arguments
$testExe = Join-Path $output 'Adaptation.Tests.exe'
Invoke-Compiler (Join-Path $CompilerRoot 'bin\clang++.exe') @('-target','x86_64-w64-mingw32','-std=c++23','-O2','-static','-municode','-DUNICODE','-D_UNICODE',(Join-Path $projectRoot 'tests\native\adaptation_tests.cpp'),'-o',$testExe,'-lbcrypt')
Invoke-Compiler $testExe @($Module,$profile)
# A second call must work without network using only the validated cache.
Invoke-Compiler $resolver @($Module,[IO.Path]::GetFullPath($CacheDirectory),(Join-Path $output 'cached.bin'),'--offline')
if ((Get-FileHash -LiteralPath $profile).Hash -ne (Get-FileHash -LiteralPath (Join-Path $output 'cached.bin')).Hash) { throw 'Cache roundtrip changed profile.' }
"Adaptation test artifacts: $output"

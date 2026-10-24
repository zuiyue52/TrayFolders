param([string]$CompilerRoot = $env:LLVM_MINGW_ROOT, [string]$WinRtInclude = '', [switch]$AppOnly)
. (Join-Path $PSScriptRoot 'Build.Common.ps1')
$objectDir = Join-Path $buildDir 'native'
$generated = Join-Path $buildDir 'generated'
New-Item -ItemType Directory -Path $binDir,$objectDir,$generated -Force | Out-Null
if (!$AppOnly) {
    if (!$CompilerRoot) { throw 'Provide -CompilerRoot pointing to LLVM-MinGW, or set LLVM_MINGW_ROOT.' }
    $clang = Join-Path $CompilerRoot 'bin\clang++.exe'
    $nativeIncludes = @()
    if ($WinRtInclude) { $nativeIncludes = @('-I', $WinRtInclude) }
    $objects = @()
    foreach ($file in @('buffer.c','hook.c','trampoline.c','hde\hde64.c')) {
        $object = Join-Path $objectDir ([IO.Path]::GetFileNameWithoutExtension($file) + '.o')
        Invoke-Compiler $clang @('-x','c','-O2','-target','x86_64-w64-mingw32','-c',(Join-Path $projectRoot ('vendor\minhook\src\' + $file)),'-o',$object)
        $objects += $object
    }
    Invoke-Compiler $clang (@('-std=c++23','-O2','-shared','-static','-DUNICODE','-D_UNICODE','-DWINVER=0x0A00','-D_WIN32_WINNT=0x0A00','-DNTDDI_VERSION=0x0A000008','-target','x86_64-w64-mingw32',(Join-Path $projectRoot 'src\native\engine.cpp')) + $nativeIncludes + $objects + @('-o',(Join-Path $binDir $project.NativeLibrary),'-lole32','-loleaut32','-lruntimeobject','-lversion','-luuid','-lcomctl32','-lbcrypt'))
    Invoke-Compiler $clang @('-std=c++23','-O2','-static','-municode','-DUNICODE','-D_UNICODE','-target','x86_64-w64-mingw32',(Join-Path $projectRoot 'src\native\symbol_resolver.cpp'),'-o',(Join-Path $binDir 'TrayFolders.Symbols.exe'),'-ldbghelp','-lwinhttp','-lbcrypt')
}
$versionSource = @"
// Generated from project.psd1; edit the manifest instead.
using System.Reflection;
[assembly: AssemblyVersion("$($project.Version)")]
[assembly: AssemblyFileVersion("$($project.Version)")]
internal static class BuildInfo { internal const string NativeLibrary = "$($project.NativeLibrary)"; }
"@
$versionFile = Join-Path $generated 'BuildInfo.g.cs'
[IO.File]::WriteAllText($versionFile,$versionSource,[Text.UTF8Encoding]::new($true))
$sources = @(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'src\app') -Filter '*.cs' -Recurse | Sort-Object FullName | ForEach-Object FullName)
$icon = Join-Path $projectRoot 'assets\tray-folders.ico'
Invoke-Compiler $csc (@('/nologo','/target:winexe','/platform:x64','/optimize+',('/win32manifest:'+(Join-Path $projectRoot 'src\app\app.manifest')),('/win32icon:'+$icon),('/resource:'+$icon+',TrayFolders.AppIcon'),('/out:'+(Join-Path $binDir 'TrayFolders.exe'))) + (Get-FrameworkReferences) + $sources + @($versionFile))
Copy-Item -LiteralPath (Join-Path $projectRoot 'src\app\app.config') -Destination (Join-Path $binDir 'TrayFolders.exe.config') -Force
Get-ChildItem -LiteralPath $binDir -Filter 'TrayFolders*' | Select-Object Name,Length

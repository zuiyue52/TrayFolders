$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$metadata = Import-PowerShellDataFile (Join-Path $root 'project.psd1')
$fixture = Join-Path $root ('build\release-tests\' + [Guid]::NewGuid().ToString('N'))
$install = Join-Path $fixture 'installed'
New-Item -ItemType Directory -Path $install -Force | Out-Null
[IO.File]::WriteAllText((Join-Path $install 'settings.ini'),"[General]`r`nFolderCount=0`r`n; personal settings preserved",[Text.Encoding]::Unicode)
[IO.File]::WriteAllText((Join-Path $install 'assignments.ini'),"[Moves]`r`nprivate=example",[Text.Encoding]::Unicode)
$before = @{}
foreach ($name in @('settings.ini','assignments.ini')) { $before[$name]=(Get-FileHash -LiteralPath (Join-Path $install $name)).Hash }
& (Join-Path $root 'scripts\Update-Local.ps1') -InstallDirectory $install -NoRestart | Out-Null
foreach ($name in $before.Keys) { if((Get-FileHash -LiteralPath (Join-Path $install $name)).Hash -ne $before[$name]) {throw "Update overwrote personal file: $name"} }
if((Get-FileHash -LiteralPath (Join-Path $install 'TrayFolders.exe')).Hash -ne (Get-FileHash -LiteralPath (Join-Path $root 'build\bin\TrayFolders.exe')).Hash){throw 'Update did not install the built application.'}
if((Get-FileHash -LiteralPath (Join-Path $install 'TrayFolders.exe.config')).Hash -ne (Get-FileHash -LiteralPath (Join-Path $root 'src\app\app.config')).Hash){throw 'Update lost the DPI configuration.'}
# A conflicting same-name native file must be rejected before stopping a process.
$conflict = Join-Path $fixture 'conflict'
New-Item -ItemType Directory -Path $conflict | Out-Null
[IO.File]::WriteAllText((Join-Path $conflict $metadata.NativeLibrary),'different native binary')
$rejected=$false
try { & (Join-Path $root 'scripts\Update-Local.ps1') -InstallDirectory $conflict -NoRestart | Out-Null }
catch { if($_.Exception.Message -like 'Native library contents changed*'){$rejected=$true}else{throw} }
if(!$rejected){throw 'Update accepted an unsafe same-name native replacement.'}
$packages = Join-Path $fixture 'packages'
$release = & (Join-Path $root 'scripts\Package.ps1') -OutputDirectory $packages
Add-Type -AssemblyName System.IO.Compression.FileSystem
foreach ($zip in @($release.SourceArchive,$release.PortableArchive)) {
    $archive=[IO.Compression.ZipFile]::OpenRead($zip)
    try {
        if(@($archive.Entries | Where-Object FullName -match '/(build|release|dist|\.git)/|icons\.tsv|command-result|\.bak$|TrayFolders\.Tests\.exe$|\.pdb$|compatibility\.bin$|\.profile$').Count){throw 'Private/runtime artifacts leaked into release.'}
        foreach ($name in @('README.md','README.en.md','LICENSE','THIRD-PARTY-NOTICES.md','manifest.sha256')) {
            if(!$archive.GetEntry('TrayFolders/'+$name)){throw "Missing public documentation: $name"}
        }
        $entry=$archive.GetEntry('TrayFolders/manifest.sha256');$reader=[IO.StreamReader]::new($entry.Open())
        try{$lines=$reader.ReadToEnd().Split("`n",[StringSplitOptions]::RemoveEmptyEntries)}finally{$reader.Dispose()}
        $fileCount=@($archive.Entries | Where-Object { $_.Name -and $_.Name -ne 'manifest.sha256' }).Count
        if ($fileCount -ne $lines.Count) { throw 'Manifest does not cover every file.' }
        foreach($line in $lines){
            $fields=$line.TrimEnd("`r") -split '  ',2
            $entry=$archive.GetEntry('TrayFolders/'+$fields[1]);if(!$entry){throw "Manifest file missing: $($fields[1])"}
            $stream=$entry.Open();$sha=[Security.Cryptography.SHA256]::Create()
            try{$hash=[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$stream.Dispose();$sha.Dispose()}
            if($hash -ne $fields[0]){throw "Manifest hash mismatch: $($fields[1])"}
        }
        if ($zip -eq $release.PortableArchive) {
            foreach ($name in @('TrayFolders.exe','TrayFolders.exe.config','TrayFolders.Symbols.exe',$metadata.NativeLibrary,'SOURCE.txt')) {
                if(!$archive.GetEntry('TrayFolders/'+$name)){throw "Runtime file missing: $name"}
            }
            foreach($name in @('settings.ini','assignments.ini')) {
                $entry=$archive.GetEntry('TrayFolders/'+$name)
                if(!$entry){throw 'Missing default configuration.'}
                $stream=$entry.Open();$sha=[Security.Cryptography.SHA256]::Create()
                try{$hash=[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$stream.Dispose();$sha.Dispose()}
                if($hash -ne (Get-FileHash -LiteralPath (Join-Path $root ('config\defaults\'+$name))).Hash){throw 'Non-default configuration in release.'}
            }
        } else {
            if(!$archive.GetEntry('TrayFolders/src/native/engine.cpp') -or !$archive.GetEntry('TrayFolders/vendor/minhook/LICENSE.txt')) { throw 'Corresponding source missing.' }
            if(@($archive.Entries | Where-Object FullName -match '\.(dll|exe)$').Count) { throw 'Binary leaked into source archive.' }
        }
    } finally { $archive.Dispose() }
}
# Exercise complete and deliberately incomplete portable installs without shell hooks.
$probe = Join-Path $fixture 'probe'
Copy-Item -LiteralPath $release.PortableDirectory -Destination $probe -Recurse
$exe = Join-Path $probe 'TrayFolders.exe'
$p = Start-Process -FilePath $exe -ArgumentList '--check-files' -WindowStyle Hidden -Wait -PassThru
if($p.ExitCode -ne 0 -or !(Get-Content (Join-Path $probe 'command-result.txt') -Raw).Contains('PASS:')){throw 'Complete portable package failed native load check.'}
Rename-Item -LiteralPath (Join-Path $probe $metadata.NativeLibrary) -NewName ($metadata.NativeLibrary + '.absent')
foreach($argument in @('--check-files','--status')) {
    $p = Start-Process -FilePath $exe -ArgumentList $argument -WindowStyle Hidden -Wait -PassThru
    if($p.ExitCode -ne 1){throw "Missing DLL produced unexpected exit code: $($p.ExitCode)"}
    $result=Get-Content (Join-Path $probe 'command-result.txt') -Raw
    if(!$result.Contains($metadata.NativeLibrary)){throw 'Missing DLL diagnostic omitted its name.'}
}
'PASS: split archives, neutral defaults, complete manifests, native load, missing-DLL cleanup and configuration-preserving update.'
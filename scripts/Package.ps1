param([string]$OutputDirectory = '')
. (Join-Path $PSScriptRoot 'Build.Common.ps1')
if (!$OutputDirectory) { $OutputDirectory = Join-Path $projectRoot 'release' }
$label = $project.ReleaseLabel
if ($label -notmatch '^[0-9]+\.[0-9]+\.[0-9]+(?:-[a-z0-9.]+)?$') { throw 'Invalid ReleaseLabel.' }
$runtime = @('TrayFolders.exe','TrayFolders.exe.config',$project.NativeLibrary,'TrayFolders.Symbols.exe')
foreach ($name in $runtime) {
    if (!(Test-Path -LiteralPath (Join-Path $binDir $name) -PathType Leaf)) { throw "Full build required; missing $name" }
}
if ((Get-Item -LiteralPath (Join-Path $binDir 'TrayFolders.exe')).VersionInfo.FileVersion -ne $project.Version) { throw 'Application build is stale; rebuild first.' }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$sourceZip = Join-Path $OutputDirectory "TrayFolders-$label-source.zip"
$portableZip = Join-Path $OutputDirectory "TrayFolders-$label-win-x64.zip"
$checksum = Join-Path $OutputDirectory 'SHA256SUMS.txt'
foreach ($path in @($sourceZip,$portableZip,$checksum)) { if (Test-Path -LiteralPath $path) { throw "Output already exists: $path" } }
$stage = Join-Path $buildDir ('package-' + [Guid]::NewGuid().ToString('N'))
$source = Join-Path $stage 'source\TrayFolders'
$portable = Join-Path $stage 'portable\TrayFolders'
New-Item -ItemType Directory -Path $source,$portable -Force | Out-Null
$common = @('README.md','README.en.md','LICENSE','THIRD-PARTY-NOTICES.md','CHANGELOG.md','CONTRIBUTING.md','docs','licenses')
# Explicit source allowlist excludes installed data, Git history, caches and builds.
foreach ($relative in ($common + @('src','tests','scripts','assets','config','vendor','project.psd1','.gitignore','.editorconfig'))) {
    Copy-Item -LiteralPath (Join-Path $projectRoot $relative) -Destination $source -Recurse
}
foreach ($relative in $common) {
    Copy-Item -LiteralPath (Join-Path $projectRoot $relative) -Destination $portable -Recurse
}
foreach ($name in $runtime) { Copy-Item -LiteralPath (Join-Path $binDir $name) -Destination $portable }
foreach ($name in @('settings.ini','assignments.ini')) { Copy-Item -LiteralPath (Join-Path $projectRoot ('config\defaults\' + $name)) -Destination $portable }
[IO.File]::WriteAllText((Join-Path $portable 'SOURCE.txt'),"Corresponding source / 对应源码: TrayFolders-$label-source.zip`r`nDistribute both archives together. / 请同时提供源码包与便携包。`r`nGPL-3.0-only; see LICENSE and THIRD-PARTY-NOTICES.md.`r`n",[Text.UTF8Encoding]::new($false))
foreach ($root in @($source,$portable)) {
    $hashes = Get-ChildItem -LiteralPath $root -Recurse -File | Sort-Object FullName | ForEach-Object {
        (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash + '  ' + [IO.Path]::GetRelativePath($root,$_.FullName).Replace('\','/')
    }
    [IO.File]::WriteAllLines((Join-Path $root 'manifest.sha256'),$hashes,[Text.UTF8Encoding]::new($false))
}
Compress-Archive -LiteralPath $source -DestinationPath $sourceZip
Compress-Archive -LiteralPath $portable -DestinationPath $portableZip
$hashes = @($sourceZip,$portableZip) | ForEach-Object { (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash + '  ' + [IO.Path]::GetFileName($_) }
[IO.File]::WriteAllLines($checksum,$hashes,[Text.UTF8Encoding]::new($false))
[pscustomobject]@{SourceDirectory=$source;PortableDirectory=$portable;SourceArchive=$sourceZip;PortableArchive=$portableZip;Checksums=$checksum}
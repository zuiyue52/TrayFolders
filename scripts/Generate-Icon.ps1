# Extract the exact .NET Framework window icon selected by the user.
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Add-Type -AssemblyName System.Windows.Forms
$form = New-Object System.Windows.Forms.Form
try {
    $stream = [IO.File]::Create((Join-Path $projectRoot 'assets\tray-folders.ico'))
    try { $form.Icon.Save($stream) } finally { $stream.Dispose() }
    $bitmap = $form.Icon.ToBitmap()
    try { $bitmap.Save((Join-Path $projectRoot 'assets\tray-folders.png')) } finally { $bitmap.Dispose() }
} finally { $form.Dispose() }

<#
.SYNOPSIS
    Safely deploys QLQPC dinput8.dll proxy into the Quake Live installation folder.

.DESCRIPTION
    1. Resolves the Quake Live directory (via -TargetDir, Steam registry/libraryfolders.vdf, or default path).
    2. Sweeps unlocked stale dinput8.dll.(old|tmp).* files in TargetDir.
    3. Skips byte-identical TargetDir\dinput8.dll via SHA-256 comparison.
    4. Stages to a temp file and uses NTFS rename-aside when dinput8.dll is locked by a running game.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$SourceDir,

    [string]$TargetDir = '',

    [string]$Config = 'Release',

    [switch]$Force
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $SourceDir -PathType Container))
{
    throw "SourceDir does not exist: $SourceDir"
}

function Resolve-QuakeLiveDir
{
    $candidates = New-Object System.Collections.Generic.List[string]
    $steamRoots = New-Object System.Collections.Generic.List[string]

    foreach ($regInfo in @(
        @{ Path = 'HKCU:\Software\Valve\Steam'; Name = 'SteamPath' },
        @{ Path = 'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam'; Name = 'InstallPath' },
        @{ Path = 'HKLM:\SOFTWARE\Valve\Steam'; Name = 'InstallPath' }
    ))
    {
        if (Test-Path -LiteralPath $regInfo.Path)
        {
            $props = Get-ItemProperty -LiteralPath $regInfo.Path -Name $regInfo.Name -ErrorAction SilentlyContinue
            $val = $props.($regInfo.Name)
            if (-not [string]::IsNullOrWhiteSpace($val))
            {
                $norm = $val.Replace('/', '\')
                if (-not $steamRoots.Contains($norm))
                {
                    $steamRoots.Add($norm)
                }
            }
        }
    }

    foreach ($defaultSteam in @("${env:ProgramFiles(x86)}\Steam", "${env:ProgramFiles}\Steam"))
    {
        if (-not [string]::IsNullOrWhiteSpace($defaultSteam) -and -not $steamRoots.Contains($defaultSteam))
        {
            $steamRoots.Add($defaultSteam)
        }
    }

    foreach ($root in $steamRoots)
    {
        $direct = Join-Path $root 'steamapps\common\Quake Live'
        if (-not $candidates.Contains($direct))
        {
            $candidates.Add($direct)
        }

        $vdf = Join-Path $root 'steamapps\libraryfolders.vdf'
        if (Test-Path -LiteralPath $vdf -PathType Leaf)
        {
            $content = Get-Content -LiteralPath $vdf -Raw -ErrorAction Stop
            foreach ($m in [regex]::Matches($content, '"path"\s+"([^"]+)"'))
            {
                $lib = $m.Groups[1].Value.Replace('\\', '\')
                $libQl = Join-Path $lib 'steamapps\common\Quake Live'
                if (-not $candidates.Contains($libQl))
                {
                    $candidates.Add($libQl)
                }
            }
        }
    }

    foreach ($dir in $candidates)
    {
        if (Test-Path -LiteralPath (Join-Path $dir 'quakelive_steam.exe') -PathType Leaf)
        {
            return $dir
        }
    }
    return $null
}

if ([string]::IsNullOrWhiteSpace($TargetDir))
{
    $TargetDir = Resolve-QuakeLiveDir
    if ([string]::IsNullOrWhiteSpace($TargetDir))
    {
        throw 'Could not locate Quake Live install folder containing quakelive_steam.exe.'
    }
}

$artifacts = @('dinput8.dll')

function Get-FileSha256([string]$Path)
{
    $share = [System.IO.FileShare]::ReadWrite -bor [System.IO.FileShare]::Delete
    $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, $share)
    try
    {
        $sha = [System.Security.Cryptography.SHA256]::Create()
        try
        {
            $hash = $sha.ComputeHash($stream)
            return ([System.BitConverter]::ToString($hash)).Replace('-', '')
        }
        finally
        {
            $sha.Dispose()
        }
    }
    finally
    {
        $stream.Dispose()
    }
}

function Confirm-FileUnlockedForReplace([string]$Path)
{
    try
    {
        $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
        $stream.Dispose()
        return $true
    }
    catch
    {
        return $false
    }
}

function Remove-MatchingFiles([string]$Dir, [string]$Pattern)
{
    $removed = 0
    if (-not (Test-Path -LiteralPath $Dir -PathType Container))
    {
        return 0
    }
    Get-ChildItem -LiteralPath $Dir -File -ErrorAction SilentlyContinue | ForEach-Object {
        if (($_.Name -imatch $Pattern) -and (Confirm-FileUnlockedForReplace -Path $_.FullName))
        {
            [System.IO.File]::Delete($_.FullName)
            $removed++
        }
    }
    return $removed
}

if (-not (Test-Path -LiteralPath $TargetDir))
{
    New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null
}

$sweepRemoved = Remove-MatchingFiles -Dir $TargetDir -Pattern '^dinput8\.dll\.(old|tmp)\.'

$results = @()
$hadInUseSwap = $false

foreach ($name in $artifacts)
{
    $src = Join-Path $SourceDir $name
    if (-not (Test-Path -LiteralPath $src -PathType Leaf))
    {
        throw "Expected built artifact missing in $SourceDir`: $name"
    }

    $dest = Join-Path $TargetDir $name

    $srcHash = Get-FileSha256 -Path $src
    if (-not $Force -and (Test-Path -LiteralPath $dest -PathType Leaf))
    {
        $destHash = Get-FileSha256 -Path $dest
        if ($destHash -and ($srcHash -eq $destHash))
        {
            $results += [pscustomobject]@{ Name = $name; Status = 'unchanged' }
            continue
        }
    }

    $tmp = Join-Path $TargetDir ("{0}.tmp.{1}" -f $name, $PID)
    [System.IO.File]::Copy($src, $tmp, $true)

    if (-not (Test-Path -LiteralPath $dest -PathType Leaf))
    {
        [System.IO.File]::Move($tmp, $dest)
        $results += [pscustomobject]@{ Name = $name; Status = 'installed' }
        continue
    }

    $replacedDirect = $false
    if (Confirm-FileUnlockedForReplace -Path $dest)
    {
        try
        {
            [System.IO.File]::Delete($dest)
            [System.IO.File]::Move($tmp, $dest)
            $replacedDirect = $true
        }
        catch [System.UnauthorizedAccessException], [System.IO.IOException]
        {
            $replacedDirect = $false
        }
    }
    if ($replacedDirect)
    {
        $results += [pscustomobject]@{ Name = $name; Status = 'updated' }
        continue
    }

    $stamp = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
    $old = Join-Path $TargetDir ("{0}.old.{1}.{2}" -f $name, $stamp, $PID)
    $swapped = $false
    [System.IO.File]::Move($dest, $old)
    try
    {
        [System.IO.File]::Move($tmp, $dest)
        $swapped = $true
    }
    catch
    {
        if (-not (Test-Path -LiteralPath $dest) -and (Test-Path -LiteralPath $old))
        {
            [System.IO.File]::Move($old, $dest)
        }
        if (Test-Path -LiteralPath $tmp -PathType Leaf)
        {
            Remove-Item -LiteralPath $tmp -Force -ErrorAction SilentlyContinue
        }
        throw
    }

    if ($swapped)
    {
        $hadInUseSwap = $true
        $results += [pscustomobject]@{ Name = $name; Status = ("replaced-in-use (old -> {0})" -f [System.IO.Path]::GetFileName($old)) }
    }
}

Write-Host ("[deploy] {0} -> {1}" -f $Config, $TargetDir)
foreach ($r in $results)
{
    Write-Host ("  {0,-22} {1}" -f $r.Name, $r.Status)
}
if ($sweepRemoved -gt 0)
{
    Write-Host ("  cleaned {0} stale backup/temp file(s)" -f $sweepRemoved)
}
if ($hadInUseSwap)
{
    if (Get-Process -Name 'quakelive_steam' -ErrorAction SilentlyContinue)
    {
        Write-Host '[deploy] Active quakelive_steam keeps running on previous code in RAM until restarted.'
    }
}

exit 0

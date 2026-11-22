param(
    [string]$HeaderPath = 'build\gen\webui\webui_embedded_assets.hpp',
    [string]$JsPath = 'src\webui\qlqpc_settings.js',
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $JsPath -PathType Leaf))
{
    throw "JS asset not found: $JsPath"
}

$regen = $Force -or
         (-not (Test-Path -LiteralPath $HeaderPath -PathType Leaf)) -or
         ((Get-Item -LiteralPath $JsPath).LastWriteTimeUtc -gt (Get-Item -LiteralPath $HeaderPath).LastWriteTimeUtc) -or
         ((Get-Item -LiteralPath $PSCommandPath).LastWriteTimeUtc -gt (Get-Item -LiteralPath $HeaderPath).LastWriteTimeUtc)

if (-not $regen)
{
    exit 0
}

$nl = [char]10

function Format-AssetBytes([string]$Path, [string]$VarName)
{
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -eq 0)
    {
        return (
            '    inline constexpr unsigned char ' + $VarName + '_data[] = { 0x00 };' + $nl +
            '    inline constexpr std::string_view ' + $VarName + '{};' + $nl
        )
    }

    $sb = New-Object System.Text.StringBuilder
    [void]$sb.Append('    inline constexpr unsigned char ' + $VarName + '_data[' + $bytes.Length + '] =' + $nl)
    [void]$sb.Append('    {' + $nl)
    for ($i = 0; $i -lt $bytes.Length; $i += 16)
    {
        $end = [Math]::Min($i + 16, $bytes.Length)
        $row = for ($j = $i; $j -lt $end; $j++)
        {
            '0x' + $bytes[$j].ToString('x2')
        }
        $line = '        ' + ($row -join ', ')
        if ($end -lt $bytes.Length)
        {
            $line += ','
        }
        [void]$sb.Append($line + $nl)
    }
    [void]$sb.Append('    };' + $nl + $nl)
    [void]$sb.Append('    inline const std::string_view ' + $VarName + '{reinterpret_cast<const char*>(' + $VarName + '_data), ' + $bytes.Length + '};' + $nl)
    return $sb.ToString()
}

$outDir = Split-Path -Parent $HeaderPath
if ($outDir -and -not (Test-Path -LiteralPath $outDir -PathType Container))
{
    New-Item -ItemType Directory -Path $outDir -Force | Out-Null
}

$content =
    '#pragma once' + $nl + $nl +
    '#include <string_view>' + $nl + $nl +
    'namespace qlqpc::webui' + $nl +
    '{' + $nl +
    (Format-AssetBytes $JsPath 'overlay_js') +
    '}' + $nl

$absHeader = if ([System.IO.Path]::IsPathRooted($HeaderPath))
{
    $HeaderPath
}
else
{
    Join-Path (Get-Location) $HeaderPath
}
[System.IO.File]::WriteAllText($absHeader, $content, (New-Object System.Text.UTF8Encoding($false)))

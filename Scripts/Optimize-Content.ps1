[CmdletBinding(SupportsShouldProcess, ConfirmImpact = 'Medium')]
param(
    [Parameter()]
    [string]$TargetDirectory = (Join-Path $PSScriptRoot '..')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$targetRoot = [System.IO.Path]::GetFullPath($TargetDirectory)
$contentRoot = Join-Path $targetRoot 'Content'
if (-not (Test-Path -LiteralPath $contentRoot -PathType Container)) {
    throw "Content folder not found under target directory: $contentRoot"
}

function Get-SourceTexturePath {
    param(
        [Parameter(Mandatory)] [string]$DdsReference,
        [Parameter(Mandatory)] [string]$JsonDirectory
    )

    $relativeCandidates = foreach ($extension in '.png', '.jpg', '.jpeg') {
        [System.Text.RegularExpressions.Regex]::Replace(
            $DdsReference,
            '\.dds$',
            $extension,
            [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
        )
    }

    foreach ($relativePath in $relativeCandidates) {
        # Asset JSON paths are normally relative to Content. Also support paths
        # relative to the JSON file so the script remains useful for new assets.
        foreach ($basePath in @($contentRoot, $JsonDirectory)) {
            $fileSystemPath = $relativePath -replace '/', [System.IO.Path]::DirectorySeparatorChar
            $candidatePath = [System.IO.Path]::GetFullPath((Join-Path $basePath $fileSystemPath))

            if ($candidatePath.StartsWith($contentRoot, [System.StringComparison]::OrdinalIgnoreCase) -and
                (Test-Path -LiteralPath $candidatePath -PathType Leaf)) {
                return $relativePath
            }
        }
    }

    return $null
}

$updatedJsonCount = 0
$updatedReferenceCount = 0
$unresolvedReferences = [System.Collections.Generic.List[string]]::new()

Get-ChildItem -LiteralPath $contentRoot -Recurse -File -Filter '*.json' | ForEach-Object {
    $jsonFile = $_
    $bytes = [System.IO.File]::ReadAllBytes($jsonFile.FullName)
    $hasUtf8Bom = $bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF
    $text = [System.IO.File]::ReadAllText($jsonFile.FullName)

    # Validate before editing. The regex only targets JSON string values (not keys)
    # whose value ends in .dds, preserving the file's original formatting.
    $null = $text | ConvertFrom-Json
    $changeState = @{ Count = 0 }
    $jsonDirectory = $jsonFile.DirectoryName
    $valuePattern = '"(?<value>(?:\\.|[^"\\])*)\.dds"(?!\s*:)' 

    $newText = [System.Text.RegularExpressions.Regex]::Replace(
        $text,
        $valuePattern,
        {
            param($match)

            $jsonString = '"' + $match.Groups['value'].Value + '.dds"'
            $ddsReference = $jsonString | ConvertFrom-Json
            $sourceReference = Get-SourceTexturePath -DdsReference $ddsReference -JsonDirectory $jsonDirectory

            if ($null -eq $sourceReference) {
                $unresolvedReferences.Add("$($jsonFile.FullName): $ddsReference")
                return $match.Value
            }

            $changeState.Count++
            $replacementJsonString = $sourceReference | ConvertTo-Json -Compress
            return $replacementJsonString
        },
        [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
    )

    if ($changeState.Count -gt 0) {
        $null = $newText | ConvertFrom-Json
        if ($PSCmdlet.ShouldProcess($jsonFile.FullName, "replace $($changeState.Count) DDS reference(s)")) {
            $encoding = [System.Text.UTF8Encoding]::new($hasUtf8Bom)
            [System.IO.File]::WriteAllText($jsonFile.FullName, $newText, $encoding)
        }
        $updatedJsonCount++
        $updatedReferenceCount += $changeState.Count
    }
}

$filesToRemove = Get-ChildItem -LiteralPath $contentRoot -Recurse -File | Where-Object {
    $_.Extension -ieq '.dds' -or
    $_.Extension -ieq '.bin' -or
    $_.Name -ilike '*.json.example'
}

foreach ($file in $filesToRemove) {
    if ($PSCmdlet.ShouldProcess($file.FullName, 'remove generated/unneeded content file')) {
        Remove-Item -LiteralPath $file.FullName -Force
    }
}

Write-Host "JSON files to update: $updatedJsonCount"
Write-Host "DDS references to replace: $updatedReferenceCount"
Write-Host "Files to remove: $($filesToRemove.Count)"

if ($unresolvedReferences.Count -gt 0) {
    Write-Warning "The following DDS references have no matching .png/.jpg/.jpeg source and were left unchanged:"
    $unresolvedReferences | ForEach-Object { Write-Warning "  $_" }
}

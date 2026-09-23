param(
    [Parameter(Mandatory = $true)]
    [string] $TargetDirectory
)

$ErrorActionPreference = "Stop"

$projectRootPath = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$targetDirectoryPath = [System.IO.Path]::GetFullPath($TargetDirectory)
$binariesDirectoryPath = [System.IO.Path]::GetFullPath((Join-Path $projectRootPath "Binaries"))
$sourceContentDirectoryPath = Join-Path $projectRootPath "Content"
$destinationContentDirectoryPath = Join-Path $targetDirectoryPath "Content"

if (Test-Path -LiteralPath $destinationContentDirectoryPath) {
    Remove-Item -LiteralPath $destinationContentDirectoryPath -Recurse -Force
}

New-Item -ItemType Directory -Path $destinationContentDirectoryPath -Force | Out-Null

Get-ChildItem -LiteralPath $sourceContentDirectoryPath -Force | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $destinationContentDirectoryPath -Recurse -Force
}

Write-Host "Content 폴더 복사 완료: $sourceContentDirectoryPath -> $destinationContentDirectoryPath"

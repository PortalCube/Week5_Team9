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
$sourceDefaultSceneDirectoryPath = Join-Path $projectRootPath "DefaultScene"
$destinationDefaultSceneDirectoryPath = Join-Path $targetDirectoryPath "DefaultScene"

if (Test-Path -LiteralPath $destinationContentDirectoryPath) {
    Remove-Item -LiteralPath $destinationContentDirectoryPath -Recurse -Force
}

if (Test-Path -LiteralPath $destinationDefaultSceneDirectoryPath) {
    Remove-Item -LiteralPath $destinationDefaultSceneDirectoryPath -Recurse -Force
}

New-Item -ItemType Directory -Path $destinationContentDirectoryPath -Force | Out-Null
New-Item -ItemType Directory -Path $destinationDefaultSceneDirectoryPath -Force | Out-Null

Get-ChildItem -LiteralPath $sourceContentDirectoryPath -Force | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $destinationContentDirectoryPath -Recurse -Force
}

Get-ChildItem -LiteralPath $sourceDefaultSceneDirectoryPath -Force | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $destinationDefaultSceneDirectoryPath -Recurse -Force
}

Write-Host "Content 폴더 복사 완료: $sourceContentDirectoryPath -> $destinationContentDirectoryPath"

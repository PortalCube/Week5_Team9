# 빌드 경로를 parameter로 받아오기
param(
    [Parameter(Mandatory = $true)]
    [string] $TargetDirectory
)

$ErrorActionPreference = "Stop"


try
{
	$postBuildDirectoryPath = Join-Path $PSScriptRoot "PostBuild"
	$postBuildScripts = Get-ChildItem -LiteralPath $postBuildDirectoryPath -Filter "*.ps1" -File |
		Sort-Object -Property Name

	foreach ($script in $postBuildScripts) {
		Write-Host "[PostBuild] Run $($script.Name)"
		& $script.FullName -TargetDirectory $TargetDirectory
	}

	Write-Host "PostBuild Completed!"
}
catch
{
    $errorFile = $_.InvocationInfo.ScriptName
    $errorLine = $_.InvocationInfo.ScriptLineNumber
	$errorFileName = Split-Path $errorFile -leaf
	

    if ([string]::IsNullOrWhiteSpace($errorFile)) {
        $errorFile = $PSCommandPath
    }

    if (-not $errorLine) {
        $errorLine = 1
    }

    # Visual Studio/MSBuild가 오류 목록 항목으로 인식하는 형식입니다.
    [Console]::Error.WriteLine(
        "$errorFile($errorLine,1): error POSTBUILD: PostBuild/$errorFileName 스크립트 오류: $($_.Exception.Message)"
    )

    exit 1
}
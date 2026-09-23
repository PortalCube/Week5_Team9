$ErrorActionPreference = "Stop"

try
{
	$preBuildDirectoryPath = Join-Path $PSScriptRoot "PreBuild"
	$preBuildScripts = Get-ChildItem -LiteralPath $preBuildDirectoryPath -Filter "*.ps1" -File |
		Sort-Object -Property Name

	foreach ($script in $preBuildScripts) {
		Write-Host "[PreBuild] Run $($script.Name)"
		& $script.FullName
	}

	Write-Host "PreBuild Completed!"
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
        "$errorFile($errorLine,1): error PREBUILD: PreBuild/$errorFileName 스크립트 오류:  $($_.Exception.Message)"
    )

    exit 1
}
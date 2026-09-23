
$ErrorActionPreference = "Stop"

$textureConverterPath = Join-Path $PSScriptRoot "texconv.exe"
$projectRootPath = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$contentDirectoryPath = Join-Path $projectRootPath "Content"


# 파일 형식
$sourceImagePatterns = @(
    (Join-Path $contentDirectoryPath "*.png")
    (Join-Path $contentDirectoryPath "*.jpg")
    (Join-Path $contentDirectoryPath "*.webp")
    (Join-Path $contentDirectoryPath "*.bmp")
    (Join-Path $contentDirectoryPath "*.gif")
)

# 실제 파일이 존재하는 패턴만 texconv에 전달
$existingImagePatterns = $sourceImagePatterns | Where-Object {
    $matchingFile = Get-ChildItem -Path $_ -Recurse -File -ErrorAction SilentlyContinue |
        Select-Object -First 1

    $null -ne $matchingFile
}


$textureConverterArguments = @(
    "-fl", "11.0"                   # D3D11의 기능 집합을 사용
    "-r:keep"						# 재귀 탐색, 파일 구조 유지
    "-m", "0"						# mipmap 자동 생성, 모든 레벨
    "-f", "R8G8B8A8_UNORM"          # DXGI 포맷을 R8G8B8A8_UNORM으로 지정
    "-y"                            # 파일이 이미 있으면 덮어쓰기
    "-o", $contentDirectoryPath     # 출력 디렉토리
) + $existingImagePatterns

Write-Host "Content 폴더의 이미지 파일을 DDS 텍스처로 변환합니다."
& $textureConverterPath @textureConverterArguments 2>&1 |
    ForEach-Object {
            Write-Host "[texconv] $_"
    }

if ($LASTEXITCODE -ne 0) {
    throw "텍스처 변환에 실패했습니다. texconv 종료 코드: $LASTEXITCODE"
}

Write-Host "텍스처 변환이 완료되었습니다."
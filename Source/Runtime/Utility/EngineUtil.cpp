#include "EngineUtil.h"

#include <Windows.h>
#include <filesystem>

FWString EngineUtil::GetContentDirectory()
{
	wchar_t ExecutablePath[MAX_PATH]{};
	const DWORD PathLength = GetModuleFileNameW(nullptr, ExecutablePath, MAX_PATH);

	if (PathLength == 0 || PathLength == MAX_PATH)
	{
		throw std::runtime_error("Failed to resolve the executable directory.");
	}

	return (std::filesystem::path(ExecutablePath).parent_path() / L"Content").wstring();
}

size_t EngineUtil::HashCombine(size_t FirstHash, size_t SecondHash)
{
	// 참고자료: boost::container_hash의 hash_combine 함수의 이전 버전 구현
	// https://www.boost.org/doc/libs/latest/libs/container_hash/doc/html/hash.html#notes_hash_combine

	// 0x9e3779b9u = 1 / φ 로 황금비를 가리킴. zero trap 방지용
	// (Bucket << 6) + (Bucket >> 2)의 경우, Bucket의 비트가 여러 부분의 해싱 연산에 영향이 갈 수 있도록 위치를 옮기는 것
	return FirstHash ^ (SecondHash + 0x9e3779b9u + (FirstHash << 6) + (FirstHash >> 2));

	// Note: 위 코드는 32비트 호환성을 위한건데, 그냥 64비트 강제할거면 엄청 간단하게 아래로 해도 됨...
	//return (Bucket << 32) ^ Index;
}

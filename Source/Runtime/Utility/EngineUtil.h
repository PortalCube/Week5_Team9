#pragma once

#include <cstddef>
#include <format>
#include <stdexcept>
#include <utility>
#include "Runtime/Core/FString.h"

namespace EngineUtil
{
	/// <summary>
	/// 실행 파일과 같은 위치에 있는 Content 폴더의 절대 경로를 반환합니다.
	/// </summary>
	FWString GetContentDirectory();

	/// <summary>
	/// 두 해시 값을 하나의 해시 값으로 만듭니다.
	/// </summary>
	/// <param name="FirstHash">해시1</param>
	/// <param name="SecondHash">해시2</param>
	/// <returns>새로 만든 해시값</returns>
	size_t HashCombine(size_t FirstHash, size_t SecondHash);

	template <typename... Args>
	std::runtime_error CreateError(std::format_string<Args...> Format, Args&&... Arguments)
	{
		return std::runtime_error(std::format(Format, std::forward<Args>(Arguments)...));
	}
}

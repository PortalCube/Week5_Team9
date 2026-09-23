#pragma once

#include <Windows.h>
#include "Runtime/Core/FString.h"

namespace WindowsUtil
{

	FString ToString(const FWString& WStr);

	FWString ToWString(const FString& Str);
};
#pragma once

#include <Windows.h>
#include "Runtime/Core/FString.h"

namespace WindowsUtil
{

	FString ToString(FWStringView WStr);

	FWString ToWString(FStringView Str);
};

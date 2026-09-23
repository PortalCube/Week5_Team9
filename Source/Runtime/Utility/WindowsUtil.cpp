#include "WindowsUtil.h"

FString WindowsUtil::ToString(const FWString& WStr)
{
	if (WStr.empty()) { return ""; }

	int SizeNeeded = WideCharToMultiByte(CP_UTF8, 0, WStr.data(), (int)WStr.size(), nullptr, 0, nullptr, nullptr);
	if (SizeNeeded == 0) { return ""; }

	FString Result(SizeNeeded, 0);
	::WideCharToMultiByte(CP_UTF8, 0, WStr.data(), (int)WStr.size(), Result.data(), SizeNeeded, nullptr, nullptr);

	return Result;
}

FWString WindowsUtil::ToWString(const FString& Str)
{
	if (Str.empty()) { return L""; }

	int SizeNeeded = MultiByteToWideChar(CP_UTF8, 0, &Str[0], (int)Str.size(), nullptr, 0);
	if (SizeNeeded == 0) { return L""; }
	
	FWString Result(SizeNeeded, 0);
	::MultiByteToWideChar(CP_UTF8, 0, Str.data(), (int)Str.size(), Result.data(), SizeNeeded);

	return Result;
}
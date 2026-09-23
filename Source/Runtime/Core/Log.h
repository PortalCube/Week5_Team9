#pragma once
#include "FString.h"
#include "TArray.h"
#include <cstdarg>
#include <cstdio>
# define UE_LOG(...) FLogManager::Get().AddLog(0, __VA_ARGS__);
# define UE_LOG_WARN(...) FLogManager::Get().AddLog(1, __VA_ARGS__);
# define UE_LOG_ERROR(...) FLogManager::Get().AddLog(2, __VA_ARGS__);
# define UE_DEBUG_LOG(...) FLogManager::Get().AddDebugLog(0, __FILE__, __LINE__, __VA_ARGS__);
# define UE_DEBUG_LOG_WARN(...) FLogManager::Get().AddDebugLog(1, __FILE__, __LINE__, __VA_ARGS__);
# define UE_DEBUG_LOG_ERROR(...) FLogManager::Get().AddDebugLog(2, __FILE__, __LINE__, __VA_ARGS__);
//# define UE_LOG(...) FLogManager::Get().AddLog(__VA_ARGS__)

class FLogManager {
private:
	TArray<FString> Logs;
public:
	static FLogManager& Get() {
		static FLogManager Instance;
		return Instance;
	}

	void AddLog(int msgType, const char* fmt, ...) {
		char buf[1024];
		int written = -1;
		if (msgType == 0) written = 0;
		else if (msgType == 1) written = std::snprintf(buf, sizeof(buf), "[Warning] ");
		else if (msgType == 2) written = std::snprintf(buf, sizeof(buf), "[ERROR] ");
		if (written < 0)
			return;
		if (written >= static_cast<int>(sizeof(buf)))
			written = static_cast<int>(sizeof(buf)) - 1;

		va_list args;
		va_start(args, fmt);
		std::vsnprintf(buf + written, sizeof(buf) - written, fmt, args);
		va_end(args);
		buf[sizeof(buf) - 1] = '\0';
		Logs.push_back(buf);
	}

	void AddDebugLog(int msgType, const char* File, int Line, const char* fmt, ...) {
		char buf[1024];
		
		const char* FileName = std::strrchr(File, '\\');

		if (FileName)
			++FileName;
		else
			FileName = File;

		const char* Slash = std::strrchr(FileName, '/');

		if (Slash)
			FileName = Slash + 1;

		int written = -1;
		if (msgType == 0) written = std::snprintf(buf, sizeof(buf), "%s, Line %d: ", FileName, Line);
		else if (msgType == 1) written = std::snprintf(buf, sizeof(buf), "[Warning] %s, Line %d: ", FileName, Line);
		else if (msgType == 2) written = std::snprintf(buf, sizeof(buf), "[ERROR] %s, Line %d: ", FileName, Line);

		if (written < 0)
			return;

		if (written >= static_cast<int>(sizeof(buf)))
			written = static_cast<int>(sizeof(buf)) - 1;

		va_list args;
		va_start(args, fmt);
		std::vsnprintf(buf + written, sizeof(buf) - written, fmt, args);
		va_end(args);
		buf[sizeof(buf) - 1] = '\0';
		Logs.push_back(buf);
	}

	const TArray<FString>& GetLogs() const {
		return Logs;
	}

	void Clear() {
		Logs.clear();
	}

	FLogManager(const FLogManager&) = delete;
	FLogManager& operator=(const FLogManager&) = delete;

	FLogManager(FLogManager&&) = delete;
	FLogManager& operator=(FLogManager&&) = delete;
private:
	FLogManager() = default;
	~FLogManager() = default;
};
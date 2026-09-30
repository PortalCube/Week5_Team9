#pragma once
#include "Editor/Core/FEditor.h"
#include "Runtime/Core/Log.h"
#include "ThirdParty/Imgui/imgui.h"

// 로그 출력과 명령어 입력을 담당하는 콘솔 창.
class FImguiConsoleWindow final
{
public:
	FImguiConsoleWindow();
	~FImguiConsoleWindow();

	// History 가 ImGui::MemAlloc 로 잡은 raw 버퍼를 소유하므로 복사를 막는다.
	FImguiConsoleWindow(const FImguiConsoleWindow&) = delete;
	FImguiConsoleWindow& operator=(const FImguiConsoleWindow&) = delete;

	void Process(FEditor& Editor, std::function<void(const char *)> f);

private:

	// 상단 메뉴바. Actions 메뉴, 레벨 토글, 필터 입력.
	// Copy 를 눌렀으면 true 를 돌려주어 로그 영역이 클립보드로 복사하게 한다.
	bool ShowMenuBar();

	// 로그가 출력되는 스크롤 영역.
	void ShowLogRegion(bool bCopyToClipboard);

	// 필터와 레벨 토글을 기준으로 로그 표시 여부를 판정한다.
	bool ShouldShowLog(const char* Log) const;

	// 한 로그에서 잘라낸 한 줄을 그린다. OriginalLog는 레벨별 색상 판정에 쓴다.
	void ShowLogLine(const char* LineBegin, const char* LineEnd, const char* OriginalLog) const;

	// 하단 명령어 입력 칸.
	void ShowCommandLine();

	// 현재 입력으로 자동완성 후보를 다시 만든다.
	void UpdateSuggestions();

	// 후보 팝업. 콘솔과 별개의 창으로 입력 칸 바로 위에 띄운다.
	void DrawSuggestionPopup(const ImVec2& InputMin);

	// 후보를 클릭해 잃은 포커스를 입력 칸으로 되돌려 달라고 요청한다.
	void RequestFocus() { bFocusInputRequested = true; }

	// InputText 콜백을 멤버 함수로 넘기기 위한 정적 우회.
	static int TextEditCallbackStub(ImGuiInputTextCallbackData* Data);
	int TextEditCallback(ImGuiInputTextCallbackData* Data);
	void ExecCommand(const char* CommandLine);

	// 레벨별 표시 토글
	bool bShowLog = true;
	bool bShowWarn = true;
	bool bShowError = true;

	// 명령어 입력
	char InputBuf[256] = {};
	ImVector<const char*> Commands;  // 자동완성 후보. 문자열 리터럴이라 해제 불필요
	ImVector<char*> History;         // Strdup 으로 잡은 버퍼. 소멸자에서 MemFree
	int HistoryPos = -1;             // -1 이면 새 줄, 0..Size-1 이면 히스토리 탐색 중

	// 자동완성 팝업
	ImVector<const char*> Suggestions;  // 이번 프레임의 후보. Commands 의 리터럴을 가리킨다
	int SuggestionIndex = -1;           // 목록이 뜨면 0 부터 시작한다
	bool bFocusInputRequested = false;  // 후보를 클릭한 다음 프레임에 입력 칸으로 포커스를 되돌린다

	// 스크롤
	ImGuiTextFilter Filter;
	bool AutoScroll = true;
	bool ScrollToBottom = false;

	// 명령 함수
	std::function<void(const char*)> ExecuteFunction;
};

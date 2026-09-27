#include "FImguiConsoleWindow.h"
#include "Runtime/Core/Log.h"
#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_internal.h"
#include "ThirdParty/Imgui/imgui_impl_dx11.h"
#include "ThirdParty/Imgui/imgui_impl_win32.h"
#include <string.h>
#include <ctime>

namespace {
	void ButtonHelper(bool& bShow, int type) {
		ImVec4 BaseColor;
		if (bShow)
			BaseColor = ImGui::GetStyleColorVec4(ImGuiCol_Header);
		else
			BaseColor = ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg);

		ImVec4 HoverColor{ BaseColor.x * 1.3f, BaseColor.y * 1.3f, BaseColor.z * 1.3f, BaseColor.w };
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HoverColor);

		const char* name = nullptr;
		switch (type) {
		case 0: name = "Log"; break;
		case 1: name = "Warning"; break;
		case 2: name = "Error"; break;
		default: break;
		}
		auto SelectableWidth = [](const char* Text)
			{
				return ImGui::CalcTextSize(Text).x;
			};
		if (ImGui::Selectable(name, bShow, 0, ImVec2(SelectableWidth(name), 0.0f))) {
			bShow = !bShow;
		}
		ImGui::PopStyleColor();
	}
}

static int   Stricmp(const char* s1, const char* s2) { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
static char* Strdup(const char* s) { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = ImGui::MemAlloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
static void  Strtrim(char* s) { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

// 대소문자 구분 없는 부분 문자열 검색. 후보를 접두사가 아니라 포함으로 찾는다.
static const char* Stristr(const char* haystack, const char* needle)
{
	if (!*needle) return haystack;
	for (; *haystack; haystack++)
	{
		const char* h = haystack;
		const char* n = needle;
		while (*h && *n && toupper((unsigned char)*h) == toupper((unsigned char)*n)) { h++; n++; }
		if (!*n) return haystack;
	}
	return nullptr;
}

void FImguiConsoleWindow::Process(FEditor& Editor, std::function<void(const char*)> f)
{
	ImGui::Begin("Console Window", nullptr, ImGuiWindowFlags_MenuBar);

	ExecuteFunction = f;

	const bool bCopyToClipboard = ShowMenuBar();

	ShowLogRegion(bCopyToClipboard);

	ImGui::Separator();

	ShowCommandLine();

	ImGui::End();
}

bool FImguiConsoleWindow::ShowMenuBar()
{
	bool bCopyToClipboard = false;

	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("Actions"))
		{
			bCopyToClipboard = ImGui::MenuItem("Copy");
			if (ImGui::MenuItem("Clear")) { FLogManager::Get().Clear(); }
			ImGui::EndMenu();
		}

		ButtonHelper(bShowLog, 0);
		ButtonHelper(bShowWarn, 1);
		ButtonHelper(bShowError, 2);

		//Filter.Draw();
		if (ImGui::InputTextWithHint("##Filter","Filter (inc,-exc)",Filter.InputBuf,IM_ARRAYSIZE(Filter.InputBuf)))
			Filter.Build();

		ImGui::EndMenuBar();
	}

	return bCopyToClipboard;
}

void FImguiConsoleWindow::ShowLogRegion(bool bCopyToClipboard)
{
	ImGuiStyle& style = ImGui::GetStyle();
	const float footer_height_to_reserve = style.SeparatorSize + style.ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar)) {
		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Clear")) FLogManager::Get().Clear();
			ImGui::EndPopup();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
		if (bCopyToClipboard)
			ImGui::LogToClipboard();

		for (const FString& item : FLogManager::Get().GetLogs())
		{
			ShowLogLine(item.c_str());
		}

		if (bCopyToClipboard)
			ImGui::LogFinish();

		// Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
		// Using a scrollbar or mouse-wheel will take away from the bottom edge.
		if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
			ImGui::SetScrollHereY(1.0f);
		ScrollToBottom = false;

		ImGui::PopStyleVar();
	}
	ImGui::EndChild();
}

void FImguiConsoleWindow::ShowLogLine(const char* Line) const
{
	if (!Filter.PassFilter(Line))
		return;

	// 레벨 토글에 걸리면 숨기고, 아니면 레벨별 색을 정한다.
	ImVec4 color;
	bool has_color = false;
	if (strstr(Line, "[ERROR]")) {
		if (!bShowError) return;
		color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true;
	}
	else if (strstr(Line, "[Warning]")) {
		if (!bShowWarn) return;
		color = ImVec4(0.6f, 0.8f, 0.4f, 1.0f); has_color = true;
	}
	else if (!bShowLog) return;
	else if (strncmp(Line, "# ", 2) == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }

	if (has_color)
		ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::TextUnformatted(Line);
	if (has_color)
		ImGui::PopStyleColor();
}

void FImguiConsoleWindow::UpdateSuggestions()
{
	Suggestions.clear();

	if (!InputBuf[0])
	{
		SuggestionIndex = -1;
		return;
	}

	const int Length = static_cast<int>(strlen(InputBuf));

	// 앞에서부터 맞는 후보를 먼저 담는다.
	for (int i = 0; i < Commands.Size; i++)
		if (Strnicmp(Commands[i], InputBuf, Length) == 0)
			Suggestions.push_back(Commands[i]);

	// 중간에 끼어 있기만 한 후보는 그 뒤로 밀어둔다. 안 그러면 "st" 를 쳤을 때
	// HI(ST)ORY 가 Stat 들보다 먼저 잡혀 엉뚱한 항목이 선택된다.
	for (int i = 0; i < Commands.Size; i++)
		if (Strnicmp(Commands[i], InputBuf, Length) != 0
			&& Stristr(Commands[i], InputBuf) != nullptr)
			Suggestions.push_back(Commands[i]);

	// 목록이 뜨는 순간 첫 항목을 골라 둔다. -1 로 두면 첫 방향키가 선택을
	// 만드는 데만 쓰여서 한 번 씹힌 것처럼 보인다.
	if (Suggestions.empty())
		SuggestionIndex = -1;
	else if (SuggestionIndex < 0 || SuggestionIndex >= Suggestions.Size)
		SuggestionIndex = 0;
}

void FImguiConsoleWindow::DrawSuggestionPopup(const ImVec2& InputMin)
{
	const ImGuiStyle& style = ImGui::GetStyle();
	const float RowHeight = ImGui::GetTextLineHeight() + style.ItemSpacing.y;
	const int VisibleCount = ImMin(Suggestions.Size, 8);
	const ImVec2 PopupSize(300.0f, RowHeight * VisibleCount + style.WindowPadding.y * 2.0f);

	// 콘솔 창과 별개의 창이라 로그 영역 레이아웃을 밀지 않는다.
	ImGui::SetNextWindowPos(ImVec2(InputMin.x, InputMin.y - PopupSize.y));
	ImGui::SetNextWindowSize(PopupSize);

	constexpr ImGuiWindowFlags PopupFlags =
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoNav;

	ImGui::Begin("##ConsoleSuggestions", nullptr, PopupFlags);

	for (int i = 0; i < Suggestions.Size; i++)
	{
		const bool bSelected = (i == SuggestionIndex);
		ImGui::PushID(i);

		if (ImGui::Selectable(Suggestions[i], bSelected))
		{
			// 클릭하면 입력 칸이 포커스를 잃어 비활성이 된다. 그래서 버퍼를
			// 직접 써도 안전하고, 다시 포커스를 받을 때 ImGui 가 버퍼에서 읽어간다.
			strcpy_s(InputBuf, IM_COUNTOF(InputBuf), Suggestions[i]);
			SuggestionIndex = i;
			RequestFocus();
		}

		// 방향키로 화면 밖 항목까지 내려가도 따라간다.
		if (bSelected)
			ImGui::SetScrollHereY();

		ImGui::PopID();
	}

	ImGui::End();
}

void FImguiConsoleWindow::ShowCommandLine()
{
	bool reclaim_focus = false;
	ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;

	// 후보를 클릭해 잃은 포커스를 되돌린다.
	if (bFocusInputRequested)
	{
		ImGui::SetKeyboardFocusHere();
		bFocusInputRequested = false;
	}

	const bool bSubmitted = ImGui::InputText("Input", InputBuf, IM_COUNTOF(InputBuf), input_text_flags, &TextEditCallbackStub, this);
	const ImVec2 InputMin = ImGui::GetItemRectMin();
	const bool bInputActive = ImGui::IsItemActive();

	if (bSubmitted)
	{
		char* s = InputBuf;
		Strtrim(s);
		if (s[0])
			ExecCommand(s);
		strcpy_s(s, 256, "");
		reclaim_focus = true;

		Suggestions.clear();
		SuggestionIndex = -1;
	}
	else if (bInputActive)
	{
		UpdateSuggestions();
	}
	else
	{
		Suggestions.clear();
		SuggestionIndex = -1;
	}

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if (reclaim_focus)
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

	if (bInputActive && !Suggestions.empty())
		DrawSuggestionPopup(InputMin);
}

FImguiConsoleWindow::FImguiConsoleWindow()
{
	FLogManager::Get().Clear();

	Commands.push_back("HELP");
	Commands.push_back("HISTORY");
	Commands.push_back("CLEAR");

	// 스탯 오버레이 토글. FEditorApplication::ExecuteCommand가 처리한다.
	Commands.push_back("Stat UNIT");
	Commands.push_back("Stat FPS");
	Commands.push_back("Stat MEMORY");
	Commands.push_back("Stat NONE");
}

int FImguiConsoleWindow::TextEditCallbackStub(ImGuiInputTextCallbackData* Data)
{
	auto* Console = static_cast<FImguiConsoleWindow*>(Data->UserData);
	return Console->TextEditCallback(Data);
}

FImguiConsoleWindow::~FImguiConsoleWindow()
{
	FLogManager::Get().Clear();
	for (int i = 0; i < History.Size; i++)
		ImGui::MemFree(History[i]);
}

int FImguiConsoleWindow::TextEditCallback(ImGuiInputTextCallbackData* data)
{
	//AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
	switch (data->EventFlag)
	{
	case ImGuiInputTextFlags_CallbackCompletion:
	{
		// 팝업이 떠 있으면 선택된 항목으로 바로 채운다.
		if (!Suggestions.empty())
		{
			const int Index = (SuggestionIndex >= 0 && SuggestionIndex < Suggestions.Size) ? SuggestionIndex : 0;
			data->DeleteChars(0, data->BufTextLen);
			data->InsertChars(0, Suggestions[Index]);
			break;
		}

		// Example of TEXT COMPLETION

		// 줄 전체를 후보와 맞춘다. 단어 단위로 끊으면 "stat u"가 공백 뒤의
		// "u"만 보게 되어 "stat unit" 같은 두 단어 명령을 못 찾는다.
		const char* word_start = data->Buf;
		const char* word_end = data->Buf + data->CursorPos;

		// Build a list of candidates
		ImVector<const char*> candidates;
		for (int i = 0; i < Commands.Size; i++)
			if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
				candidates.push_back(Commands[i]);

		if (candidates.Size == 0)
		{
			// No match
			UE_LOG("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
		}
		else if (candidates.Size == 1)
		{
			// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
			data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
			data->InsertChars(data->CursorPos, candidates[0]);
			data->InsertChars(data->CursorPos, " ");
		}
		else
		{
			// Multiple matches. Complete as much as we can..
			// So inputting "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
			int match_len = (int)(word_end - word_start);
			for (;;)
			{
				int c = 0;
				bool all_candidates_matches = true;
				for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
					if (i == 0)
						c = toupper(candidates[i][match_len]);
					else if (c == 0 || c != toupper(candidates[i][match_len]))
						all_candidates_matches = false;
				if (!all_candidates_matches)
					break;
				match_len++;
			}

			if (match_len > 0)
			{
				data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
				data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
			}

			// List matches
			UE_LOG("Possible matches:\n");
			for (int i = 0; i < candidates.Size; i++)
				UE_LOG("- %s\n", candidates[i]);
		}

		break;
	}
	case ImGuiInputTextFlags_CallbackHistory:
	{
		// 드롭다운이 떠 있으면 위아래 키는 후보 이동으로 쓴다.
		if (Suggestions.Size > 0)
		{
			if (data->EventKey == ImGuiKey_UpArrow)
				SuggestionIndex = (SuggestionIndex <= 0) ? Suggestions.Size - 1 : SuggestionIndex - 1;
			else if (data->EventKey == ImGuiKey_DownArrow)
				SuggestionIndex = (SuggestionIndex + 1 >= Suggestions.Size) ? 0 : SuggestionIndex + 1;

			break;
		}

		// Example of HISTORY
		const int prev_history_pos = HistoryPos;
		if (data->EventKey == ImGuiKey_UpArrow)
		{
			if (HistoryPos == -1)
				HistoryPos = History.Size - 1;
			else if (HistoryPos > 0)
				HistoryPos--;
		}
		else if (data->EventKey == ImGuiKey_DownArrow)
		{
			if (HistoryPos != -1)
				if (++HistoryPos >= History.Size)
					HistoryPos = -1;
		}

		// A better implementation would preserve the data on the current input line along with cursor position.
		if (prev_history_pos != HistoryPos)
		{
			const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
			data->DeleteChars(0, data->BufTextLen);
			data->InsertChars(0, history_str);
		}
	}
	}
	return 0;
}

void FImguiConsoleWindow::ExecCommand(const char* command_line)
{
	UE_LOG("# %s\n", command_line);

	// Insert into history. First find match and delete it so it can be pushed to the back.
	// This isn't trying to be smart or optimal.
	HistoryPos = -1;
	for (int i = History.Size - 1; i >= 0; i--)
		if (Stricmp(History[i], command_line) == 0)
		{
			ImGui::MemFree(History[i]);
			History.erase(History.begin() + i);
			break;
		}
	History.push_back(Strdup(command_line));

	// Process command
	if (Stricmp(command_line, "CLEAR") == 0)
	{
		FLogManager::Get().Clear();
	}
	else if (Stricmp(command_line, "HELP") == 0)
	{
		UE_LOG("Commands:");
		for (int i = 0; i < Commands.Size; i++)
			UE_LOG("- %s", Commands[i]);
	}
	else if (Stricmp(command_line, "HISTORY") == 0)
	{
		int first = History.Size - 10;
		for (int i = first > 0 ? first : 0; i < History.Size; i++)
			UE_LOG("%3d: %s\n", i, History[i]);
	}
	else
	{
		// 이외의 커맨드는 EditorApplication의 함수로 전달합니다.
		ExecuteFunction(command_line);
	}

	// On command input, we scroll to bottom even if AutoScroll==false
	ScrollToBottom = true;
}


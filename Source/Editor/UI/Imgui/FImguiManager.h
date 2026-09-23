#pragma once

#include "Runtime/Rendering/FRenderer.h"
#include "ThirdParty/Imgui/imgui.h"

//상속을 막는 final
class FImguiManager final
{
private:
	enum class EImplType {
		Win32DX11,
		NOT_IMPLEMENTED
	};
	EImplType ImplType = EImplType::NOT_IMPLEMENTED;
	ImGuiID EditorViewportID = 0;

public:
	bool Initialize_ImplWin32DX11(HWND& Window, ID3D11Device* Device, ID3D11DeviceContext* Context);
	void NewFrame();
	void RenderUI();
	[[nodiscard]] ImGuiID GetEditorViewportID() const { return EditorViewportID; }
};
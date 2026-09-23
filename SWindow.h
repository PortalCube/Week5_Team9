#pragma once

// SSplitterH : 자식을 수평 나열 (좌|우). 분할선은 세로. 마우스 X로 드래그.
// SSplitterV : 자식을 수직 나열 (상/하). 분할선은 가로. 마우스 Y로 드래그.
//   ※ 이름 = 자식 배치 방향 (UE Slate EOrientation 규약)
//
//        SSplitterH                    SSplitterV
//   ┌───────┃───────┐            ┌───────────────┐
//   │				 ┃				 │            │				  		    	│
//   │ SideLT		 ┃SideRB		 │            │    SideLT    		    		│
//   │ (Left)		 ┃(Right)		 │            ├━━━━━━━━━━━━━━━┤
//   │				 ┃				 │            │    SideRB     
//   └───────┃───────┘             └───────────────┘
struct FRect
{
	float Left, Top, Right, Bottom; 
	float GetWidth() const { return Right - Left; }
	float GetHeight() const { return Bottom - Top; }

};
class SWindow
{	
public:
	FRect Rect;
	int32 ViewportIndex = -1;   // -1 = 스플리터, 0 이상 = 뷰포트 리프
	virtual ~SWindow() = default;         // 파생을 포인터로 다루니 가상 소멸자
	//bool ISHover();
	
	bool bisActive = false;
	virtual void OnResize(const FRect& In) { Rect = In; }
};

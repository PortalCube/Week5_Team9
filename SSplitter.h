#pragma once
#include "SWindow.h"

/*
SSplitterH(좌 | 우)					  ┌──────────┬
├─ SSplitterV(좌를 상 / 하)         │  Top     │  Persp   
│   ├─ Top                         ├──────────┼
│   └─ Front                       │  Front   │  Right   
└─ SSplitterV(우를 상 / 하)         └──────────┴
├─ Persp                            ↑세로선 1개(H)
└─ Right                        가로선 2개(V)
*/
class SSplitter : public SWindow
{
public:
	~SSplitter() override { }
	SWindow* SideLT=nullptr; // Left or Top
	SWindow* SideRB=nullptr; // Right or Bottom
	float Ratio=0.5f;

	bool isSplitter = false;

};
// SSplitterH: 자식을 수평 나열(좌 | 우).분할선은 세로.마우스 X로 드래그.
class SSplitterH : public SSplitter 
{
public:
	void OnResize(const FRect& In) override
	{
		Rect = In;
		const float SplitX = In.Left + In.GetWidth() * Ratio;
		if (SideLT && SideLT->bisActive) SideLT->OnResize({In.Left,In.Top,SplitX,In.Bottom});
		if (SideRB && SideRB->bisActive) SideRB->OnResize({ SplitX,In.Top,In.Right,In.Bottom });
	}
};

// SSplitterV : 자식을 수직 나열 (상/하). 분할선은 가로. 마우스 Y로 드래그.
class SSplitterV : public SSplitter 
{
public:
	void OnResize(const FRect& In) override
	{
		Rect = In;

		const float SplitY = In.Top + In.GetHeight() * Ratio;
		if (SideLT && SideLT->bisActive) SideLT->OnResize({ In.Left,In.Top,In.Right,SplitY});
		if (SideRB && SideRB->bisActive) SideRB->OnResize({ In.Left,SplitY,In.Right,In.Bottom });

	}
};


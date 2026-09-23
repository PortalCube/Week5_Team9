#include "UAnimatedBillboardComp.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/Engine/FArchive.h"
#include "UClass.h"
#include <algorithm>

IMPLEMENT_UCLASS(UAnimatedBillboardComp, UBillBoardComp)
UCLASS_META(UAnimatedBillboardComp, DisplayName, "AnimatedBillboard")

void UAnimatedBillboardComp::SetSpriteSheet(int InGridX, int InGridY,
                                            float InFrameRate,
                                            int InTotalFrames) {
  GridX = (InGridX > 0) ? InGridX : 1;
  GridY = (InGridY > 0) ? InGridY : 1;
  FrameRate = (InFrameRate > 0.0f) ? InFrameRate : 1.0f;

  if (InTotalFrames > 0) {
    TotalFrames = InTotalFrames;
  } else {
    TotalFrames = GridX * GridY;
  }

  CurrentFrame = 0;
  ElapsedTime = 0.0f;
  RefreshUV();
}

void UAnimatedBillboardComp::Stop() {
  bPlaying = false;
  CurrentFrame = 0;
  ElapsedTime = 0.0f;
  RefreshUV();
}

void UAnimatedBillboardComp::SetCurrentFrame(int InFrame) {
  if (TotalFrames > 0) {
    CurrentFrame = std::clamp(InFrame, 0, TotalFrames - 1);
    RefreshUV();
  }
}

void UAnimatedBillboardComp::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);

	Archive.SetInt32("GridX", GridX);
	Archive.SetInt32("GridY", GridY);
	Archive.SetInt32("TotalFrames", TotalFrames);
	//Archive.SetInt32("CurrentFrame", CurrentFrame);
	Archive.SetFloat("FrameRate", FrameRate);
	//Archive.SetFloat("ElapsedTime", ElapsedTime);
	//Archive.SetBool("Playing", bPlaying);
	Archive.SetBool("Loop", bLoop);
	Archive.SetVector2("CurrentUVScale", CurrentUVScale);
	Archive.SetVector2("CurrentUVOffset", CurrentUVOffset);
}

void UAnimatedBillboardComp::Deserialize(const FArchive& Archive)
{
	Super::Deserialize(Archive);

	GridX = Archive.GetInt32("GridX");
	GridY = Archive.GetInt32("GridY");
	TotalFrames = Archive.GetInt32("TotalFrames");
	FrameRate = Archive.GetFloat("FrameRate");
	bLoop = Archive.GetBool("Loop");
	CurrentUVScale = Archive.GetVector2("CurrentUVScale");
	CurrentUVOffset = Archive.GetVector2("CurrentUVOffset");
}

void UAnimatedBillboardComp::Update(float DeltaTime) {
  if (!bPlaying || TotalFrames <= 1 || FrameRate <= 0.0f) {
    return;
  }

  ElapsedTime += DeltaTime;
  const float FrameDuration = 1.0f / FrameRate;

  while (ElapsedTime >= FrameDuration) {
    ElapsedTime -= FrameDuration;
    CurrentFrame++;

    if (CurrentFrame >= TotalFrames) {
      if (bLoop) {
        CurrentFrame = 0;
      } else {
        CurrentFrame = TotalFrames - 1;
        bPlaying = false;
        break;
      }
    }
  }

  RefreshUV();
}



void UAnimatedBillboardComp::RefreshUV()
{
  if (GridX <= 0 || GridY <= 0) {
    SetUVScale(FVector2{1.0f, 1.0f});
    SetUVOffset(FVector2{0.0f, 0.0f});
    return;
  }

  const FVector2 UVScale{
      1.0f / static_cast<float>(GridX),
      1.0f / static_cast<float>(GridY)};

  const int Col = CurrentFrame % GridX;
  const int Row = CurrentFrame / GridX;

  SetUVScale(UVScale);
  SetUVOffset(FVector2{
      static_cast<float>(Col) * UVScale.X,
      static_cast<float>(Row) * UVScale.Y});
}

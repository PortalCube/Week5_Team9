#pragma once

#include "UBillBoardComp.h"

class FArchive;

// 애니메이션 빌보드 컴포넌트
class UAnimatedBillboardComp : public UBillBoardComp {
  DECLARE_UCLASS(UAnimatedBillboardComp, UBillBoardComp)
  GENERATED_BODY()

protected:
  explicit UAnimatedBillboardComp() = default;

  virtual void Serialize(FArchive& Archive) const override;
  virtual void Deserialize(const FArchive& Archive) override;

public:
  // 매 프레임 애니메이션 갱신
  void Update(float DeltaTime) override;

  // 스프라이트 시트 설정
  void SetSpriteSheet(int InGridX, int InGridY, float InFrameRate = 10.0f,
                      int InTotalFrames = -1);

  void Play() { bPlaying = true; }
  void Pause() { bPlaying = false; }
  void Stop();

  void SetCurrentFrame(int InFrame);
  int GetCurrentFrame() const { return CurrentFrame; }
  int GetGridX() const { return GridX; }
  int GetGridY() const { return GridY; }
  int GetTotalFrames() const { return TotalFrames; }
  void SetFrameRate(float InRate) { FrameRate = InRate; }
  float GetFrameRate() const { return FrameRate; }


  void SetLooping(bool bInLoop) { bLoop = bInLoop; }
  bool IsLooping() const { return bLoop; }
  bool IsPlaying() const { return bPlaying; }


private:
  // 좌표 계산
  void RefreshUV();

private:
  int GridX = 1;
  int GridY = 1;
  int TotalFrames = 1;
  int CurrentFrame = 0;

  float FrameRate = 10.0f;
  float ElapsedTime = 0.0f;

  bool bPlaying = true;
  bool bLoop = true;

  FVector2 CurrentUVScale{1.0f, 1.0f};
  FVector2 CurrentUVOffset{0.0f, 0.0f};
};


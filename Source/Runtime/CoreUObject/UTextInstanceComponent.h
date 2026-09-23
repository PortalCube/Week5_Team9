#pragma once

#include "Runtime/CoreUObject/UInstancePrimitiveComponent.h"
#include "Runtime/Rendering/FFont.h"
#include "Runtime/Rendering/FRenderQueue.h"


class FArchive;
class UFont;

class UTextInstanceComponent : public UInstancePrimitiveComponent {
  GENERATED_BODY()
  DECLARE_UCLASS(UTextInstanceComponent, UInstancePrimitiveComponent)

public:
  void Initialize() override;
  void Update(float delta) override;

  void SetText(const FWString &InText);

  [[nodiscard]] const FWString &GetText() const { return Text; }
  //void SetFont(TSharedPtr<FFont> InFont);

  void SetFont(const FName& InName);
  void SetFont(UFont* InFont);
  UFont* GetFont() const { return FontAsset; }

  void RebuildTextMesh();

  // Object -> World 변환 행렬 생성
  virtual FMatrix GetRenderMatrix(const FCamera &Camera) const override;
  virtual const FRenderData& GetRenderData(const FCamera &Camera) const override;

  virtual EEngineShowFlags GetShowFlag() const {
    return EEngineShowFlags::SF_BillboardText;
  }

  virtual void Serialize(FArchive &Archive) const override;
  virtual void Deserialize(const FArchive &Archive) override;

  float GetWidth() const { return Width; }
  float GetHeight() const { return Height; }

private:
  TSharedPtr<FFont> Font;
  UFont* FontAsset = nullptr;
  FWString Text = L"Hello Jungle World!";

  float Width = 0.0f;
  float Height = 0.0f;

  TArray<FInstanceData> Instances;
};

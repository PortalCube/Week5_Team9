#pragma once
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Math/FQuaternion.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "UPrimitiveComponent.h"

class UScene;
class FArchive;

class UBillBoardComp : public UPrimitiveComponent {
  DECLARE_UCLASS(UBillBoardComp, UPrimitiveComponent)
  GENERATED_BODY()

protected:
  explicit UBillBoardComp() = default;

  virtual void Serialize(FArchive& Archive) const;
  virtual void Deserialize(const FArchive& Archive);

public:
  void Initialize() override;

  virtual void SetTexture(UTexture* Texture);
  UTexture* GetTexture() const;

  // Object -> World 변환 행렬 생성
  virtual FMatrix GetRenderMatrix(const FCamera& Camera) const override;
  
  virtual EEngineShowFlags GetShowFlag() const { return EEngineShowFlags::SF_BillboardText; }

  void SetUVScale(FVector2 Value);
  void SetUVOffset(FVector2 Value);

  FVector2 GetUVScale() const;
  FVector2 GetUVOffset() const;

};


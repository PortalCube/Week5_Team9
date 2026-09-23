#include "UTextInstanceComponent.h"
#include "Runtime/Asset/UFont.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "UClass.h"
#include <algorithm>
#include <limits>
#include <windows.h>


IMPLEMENT_UCLASS(UTextInstanceComponent, UInstancePrimitiveComponent)

namespace {
FMatrix GetRenderMatrix(const FTransform &Transform, const FCamera &Camera) {
  FMatrix CameraRotation = Camera.GetRotationMatrix();
  FVector ViewForward =
      CameraRotation.TransformPointRow(FVector{1.0f, 0.0f, 0.0f}, 0.0f); // X+
  FVector ViewRight =
      CameraRotation.TransformPointRow(FVector{0.0f, 1.0f, 0.0f}, 0.0f); // Y+
  FVector ViewUp =
      CameraRotation.TransformPointRow(FVector{0.0f, 0.0f, 1.0f}, 0.0f); // Z+

  FVector Up = ViewUp * Transform.Scale3D.Z;
  FVector Right = ViewRight * Transform.Scale3D.Y;

  return FMatrix{
      FVector4{ ViewForward, 0.0f },
      FVector4{ Right, 0.0f },
      FVector4{ Up, 0.0f },
      FVector4{ Transform.Location, 1.0f },
  };
}
} // namespace

void UTextInstanceComponent::Initialize() {
  Super::Initialize();

  FAssetRegistry& Registry = FAssetRegistry::GetInstance();
  SetMesh(Registry.Get<UStaticMesh>("#Rect"));
  SetMaterial(Registry.Get<UMaterial>("Material/Text.json"));
  SetFont(Registry.Get<UFont>("Font/BazziOTF.json"));

  RenderData.Type = ERenderType::Text;

  RebuildTextMesh();
}

void UTextInstanceComponent::Update(float delta) {}

void UTextInstanceComponent::SetText(const FWString &InText) {
  Text = InText;
  RebuildTextMesh();
}

//void UTextInstanceComponent::SetFont(TSharedPtr<FFont> InFont) {
//  Font = InFont;
//  RenderData.TextureId = FName("bazziotf");
//  RebuildTextMesh();
//}

void UTextInstanceComponent::SetFont(const FName& InName) {
  Font = FRenderResourceLibrary::Get().GetFont(InName);
  FontAsset = nullptr;
  //RenderData.TextureId(InName);
  RebuildTextMesh();
}

void UTextInstanceComponent::SetFont(UFont* InFont) {
  if (!InFont || !InFont->Get()) {
    return;
  }

  FontAsset = InFont;
  Font = FRenderResourceLibrary::Get().GetFont(
      std::filesystem::path(InFont->GetID().ToString()).stem().string());
  SetTexture(InFont->GetTexture());
  RebuildTextMesh();
}

void UTextInstanceComponent::RebuildTextMesh() {
  Instances.clear();
  Width = 0;
  Height = 0;

  if (Text.empty() || !Font)
    return;

  // Phase 1 : 전체 Bound 값 계산
  float minY = std::numeric_limits<float>::max();
  float minZ = std::numeric_limits<float>::max();

  float maxY = std::numeric_limits<float>::lowest();
  float maxZ = std::numeric_limits<float>::lowest();

  // 현재 텍스트에 글리프가 단 하나라도 있는지 체크
  bool bHasVisibleGlyph = false;

  float prevAdvance = 0.0f;
  for (uint16 i = 0; i < Text.length(); ++i) {
    const FCharacterInfo &CharInfo = Font->GetCharInfo(Text.at(i));
    if (Text.at(i) == ' ' || Text.at(i) == '\t') {
      prevAdvance += CharInfo.advance;
      continue;
    }

    const float glyphLeft = CharInfo.planeLeft + prevAdvance;
    const float glyphRight = CharInfo.planeRight + prevAdvance;
    const float glyphTop = -CharInfo.planeTop;
    const float glyphBottom = -CharInfo.planeBottom;

    minY = std::min(minY, std::min(glyphLeft, glyphRight));
    minZ = std::min(minZ, std::min(glyphTop, glyphBottom));

    maxY = std::max(maxY, std::max(glyphLeft, glyphRight));
    maxZ = std::max(maxZ, std::max(glyphTop, glyphBottom));

    bHasVisibleGlyph = true;

    prevAdvance += CharInfo.advance;
  }

  // 글리프가 없다면 렌더링할 데이터가 없음
  if (!bHasVisibleGlyph)
    return;

  Width = maxY - minY;
  Height = maxZ - minZ;
  const float textCenterY = (minY + maxY) * 0.5f;
  const float textCenterZ = (minZ + maxZ) * 0.5f;

  // Phase 2: 실제 FInstanceData 계산

  prevAdvance = 0.0f;
  for (uint16 i = 0; i < Text.length(); ++i) {
    const FCharacterInfo &CharInfo = Font->GetCharInfo(Text.at(i));
    if (Text.at(i) == ' ' ||
        Text.at(i) ==
            '\t') { // 공백일 경우 인스턴스를 생성하지 않고 위치만 누적
      prevAdvance += CharInfo.advance;
      continue;
    }
    // 원본과 동일하게 4개 정점 좌표 및 UV 계산
    FVertexData tv[4]{};
    tv[0].x = 0.0f;
    tv[0].y = CharInfo.planeLeft + prevAdvance;
    tv[0].z = -CharInfo.planeTop;
    tv[0].u = CharInfo.u;
    tv[0].v = CharInfo.v;

    tv[1].x = 0.0f;
    tv[1].y = CharInfo.planeRight + prevAdvance;
    tv[1].z = -CharInfo.planeTop;
    tv[1].u = CharInfo.u + CharInfo.width;
    tv[1].v = CharInfo.v;

    tv[2].x = 0.0f;
    tv[2].y = CharInfo.planeLeft + prevAdvance;
    tv[2].z = -CharInfo.planeBottom;
    tv[2].u = CharInfo.u;
    tv[2].v = CharInfo.v + CharInfo.height;

    tv[3].x = 0.0f;
    tv[3].y = CharInfo.planeRight + prevAdvance;
    tv[3].z = -CharInfo.planeBottom;
    tv[3].u = CharInfo.u + CharInfo.width;
    tv[3].v = CharInfo.v + CharInfo.height;

    float charWidth = tv[1].y - tv[0].y;
    float charHeight = tv[0].z - tv[2].z; // planeTop - planeBottom
    float centerY = (tv[0].y + tv[1].y) * 0.5f - textCenterY;
    float centerZ = (tv[0].z + tv[2].z) * 0.5f - textCenterZ;

    FMatrix CharMatrix =
        FMatrix::MakeScale(FVector(1.0f, charWidth, charHeight)) *
        FMatrix::MakeTranslation(FVector(0.0f, centerY, centerZ));

    // 글자별 순수 로컬 변환 (크기 * 위치)
    FInstanceData Data{
        .World = CharMatrix,
        .Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f),
        .UVScale = FVector2(CharInfo.width, CharInfo.height),
        .UVOffset = FVector2(tv[0].u, tv[0].v),
    };

    Instances.push_back(Data);
    prevAdvance += CharInfo.advance;
  }
}

FMatrix UTextInstanceComponent::GetRenderMatrix(const FCamera &Camera) const {
  FTransform Transform = GetGlobalTransform();

  FMatrix ScaleTransform = FMatrix::MakeScale({1.0f, Width, Height});
  FMatrix ModelMatrix = ::GetRenderMatrix(Transform, Camera);

  return ScaleTransform * ModelMatrix;
}

const FRenderData& UTextInstanceComponent::GetRenderData(const FCamera &Camera) const {

  TArray<FInstanceData> Built;

  FTransform Transform = GetGlobalTransform();
  FMatrix ModelMatrix = ::GetRenderMatrix(Transform, Camera);

  // 글자별 FInstanceData에 빌보드 월드 행렬 적용
  for (const FInstanceData &Inst : Instances) {
    FInstanceData WorldInst = Inst;
    WorldInst.World *= ModelMatrix;
    Built.push_back(WorldInst);
  }

  RenderData.Instances = std::move(Built);

  return RenderData;
}

void UTextInstanceComponent::Serialize(FArchive &Archive) const {
  Super::Serialize(Archive);

  Archive.SetWString("Text", Text);

  if (FontAsset) {
    Archive.SetString("FontAsset", FontAsset->GetID().ToString());
  }
}

void UTextInstanceComponent::Deserialize(const FArchive &Archive) {
  Super::Deserialize(Archive);

  Text = Archive.GetWString("Text");

  if (!Archive.IsNull("FontAsset")) {
    FAssetRegistry &Registry = FAssetRegistry::GetInstance();
    FString FontAssetID = Archive.GetString("FontAsset");
    UFont *LoadedFont = Registry.Get<UFont>(FontAssetID);

    if (LoadedFont) {
      SetFont(LoadedFont);
      return;
    }
  }

  RebuildTextMesh();
}

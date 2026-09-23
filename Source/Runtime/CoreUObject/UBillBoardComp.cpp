#include "UBillBoardComp.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Engine/FSceneView.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Asset/UTexture.h"
#include "UClass.h"
#include <algorithm>
#include <cctype>

IMPLEMENT_UCLASS(UBillBoardComp, UPrimitiveComponent)
UCLASS_META(UBillBoardComp, DisplayName, "BillBoard")
UCLASS_META(UBillBoardComp, MeshName, "BillBoard")

void UBillBoardComp::Initialize() {
  Super::Initialize();

  FAssetRegistry& Registry = FAssetRegistry::GetInstance();
  SetMesh(Registry.Get<UStaticMesh>("#Rect"));
  SetMaterial(Registry.Get<UMaterial>("Material/Billboard.json"));

  RenderData.Type = ERenderType::Texture;
}

void UBillBoardComp::Serialize(FArchive& Archive) const
{
    Super::Serialize(Archive);

    UTexture* Texture = GetTexture();
    if (Texture)
    {
        Archive.SetString("TextureAsset", Texture->GetID().ToString());
    }
}

void UBillBoardComp::Deserialize(const FArchive& Archive)
{
    Super::Deserialize(Archive);

    if (Archive.IsNull("TextureAsset"))
    {
        return;
    }

    FAssetRegistry& Registry = FAssetRegistry::GetInstance();
    FString TextureAssetID = Archive.GetString("TextureAsset");
    UTexture* Texture = Registry.Get<UTexture>(TextureAssetID);

    if (Texture)
    {
        SetTexture(Texture);
    }
}

void UBillBoardComp::SetTexture(UTexture* Texture)
{
    RenderData.Materials[0].Texture = Texture;
}

UTexture* UBillBoardComp::GetTexture() const
{
    return RenderData.Materials.empty() ? nullptr : RenderData.Materials[0].Texture;
}

FMatrix UBillBoardComp::GetRenderMatrix(const FCamera& Camera) const
{
    FTransform Transform = GetGlobalTransform();

    FMatrix CameraRotation = Camera.GetRotationMatrix();
    FVector ViewForward = CameraRotation.TransformPointRow(FVector{ 1.0f, 0.0f, 0.0f }, 0.0f); // X+
    FVector ViewRight = CameraRotation.TransformPointRow(FVector{ 0.0f, 1.0f, 0.0f }, 0.0f); // Y+
    FVector ViewUp = CameraRotation.TransformPointRow(FVector{ 0.0f, 0.0f, 1.0f }, 0.0f); // Z+

    FVector Up = ViewUp * Transform.Scale3D.Z;
    FVector Right = ViewRight * Transform.Scale3D.Y;

    return FMatrix
    {
        FVector4{ ViewForward, 0.0f },
        FVector4{ Right, 0.0f },
        FVector4{ Up, 0.0f },
        FVector4{ Transform.Location, 1.0f },
    };
}

void UBillBoardComp::SetUVScale(FVector2 Value)
{
    RenderData.Materials[0].UVScale = Value;
}

void UBillBoardComp::SetUVOffset(FVector2 Value)
{
    RenderData.Materials[0].UVOffset = Value;
}

FVector2 UBillBoardComp::GetUVScale() const
{
    return RenderData.Materials[0].UVScale;
}

FVector2 UBillBoardComp::GetUVOffset() const
{
    return RenderData.Materials[0].UVOffset;
}

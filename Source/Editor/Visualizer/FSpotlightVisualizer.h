#pragma once

#include "Editor/Visualizer/IVisualizer.h"

class FSpotlightVisualizer : public IVisualizer
{
public:
    void Draw(
        const UPrimitiveComponent& Component,
        FRenderView& RenderView,
        const FCamera& Camera,
        const FVector4& Color
    ) const override;
};

#pragma once

class FRenderer;
class FRenderResourceLibrary;

namespace MeshUtil
{
bool CreateCubeMesh(FRenderer &Renderer, FRenderResourceLibrary &Library);
bool CreateCylinderMesh(
    FRenderer &Renderer,
    FRenderResourceLibrary &Library,
    float Height,
    unsigned int SliceCount,
    float TopRadius,
    float BottomRadius
);
bool CreateConeMesh(FRenderer &Renderer, FRenderResourceLibrary &Library);
bool CreateSpotlightConeMesh(
    FRenderer &Renderer,
    FRenderResourceLibrary &Library
);
bool CreateArrowMesh(FRenderer &Renderer, FRenderResourceLibrary &Library);
bool CreateCircleMesh(FRenderer &Renderer, FRenderResourceLibrary &Library);
bool CreateRotationGizmoMesh(
    FRenderer &Renderer,
    FRenderResourceLibrary &Library
);
bool CreateSquareArrowMesh(
    FRenderer &Renderer,
    FRenderResourceLibrary &Library
);
bool CreateGridMesh(FRenderer &Renderer, FRenderResourceLibrary &Library);
bool CreateSphereMesh(FRenderer &Renderer, FRenderResourceLibrary &Library);
bool CreateLineMesh(FRenderer &Renderer, FRenderResourceLibrary &Library);
bool CreatePlaneMesh(FRenderer &Renderer, FRenderResourceLibrary &Library);
bool CreateRectMesh(FRenderer &Renderer, FRenderResourceLibrary &Library);
}

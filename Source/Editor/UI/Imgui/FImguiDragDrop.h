#pragma once

#include "Runtime/Asset/UAsset.h";

struct FContentDragPayload
{
    UAsset* Ptr;
};

inline constexpr const char* ContentDragPayloadType = "ENGINE_CONTENT";
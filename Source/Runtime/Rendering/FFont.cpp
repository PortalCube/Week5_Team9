#include "FFont.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Core/IntTypes.h"

FFont::FFont(const FArchive& Archive)
{
	const FArchive AtlasArchive = Archive.GetArchive("atlas");
	const float AtlasWidth = static_cast<float>(AtlasArchive.GetUInt32("width"));
	const float AtlasHeight = static_cast<float>(AtlasArchive.GetUInt32("height"));

	for (const FArchive& GlyphArchive : Archive.GetArchiveArray("glyphs"))
	{
		FCharacterInfo Info{};
		const uint32 Unicode = GlyphArchive.GetUInt32("unicode");
		Info.advance = GlyphArchive.GetFloat("advance");

		if (!GlyphArchive.IsNull("planeBounds"))
		{
			const FArchive Bounds = GlyphArchive.GetArchive("planeBounds");
			Info.planeLeft = Bounds.GetFloat("left");
			Info.planeTop = Bounds.GetFloat("top");
			Info.planeRight = Bounds.GetFloat("right");
			Info.planeBottom = Bounds.GetFloat("bottom");
		}

		if (!GlyphArchive.IsNull("atlasBounds"))
		{
			const FArchive Bounds = GlyphArchive.GetArchive("atlasBounds");
			const float Left = Bounds.GetFloat("left");
			const float Top = Bounds.GetFloat("top");
			const float Right = Bounds.GetFloat("right");
			const float Bottom = Bounds.GetFloat("bottom");

			Info.u = Left / AtlasWidth;
			Info.v = Top / AtlasHeight;
			Info.width = (Right - Left) / AtlasWidth;
			Info.height = (Bottom - Top) / AtlasHeight;
		}

		CharInfoMap.emplace(static_cast<char32_t>(Unicode), Info);
	}
}

void FFont::InitializeForASCII(float InNumberOfLine)
{
	// 16x16 코드페이지 437 기준
	float uvSize = 1.0f / InNumberOfLine;
	for (uint16 i = 0; i < 256; ++i)
	{
		uint16 col = i % 16;
		uint16 row = i / 16;

		FCharacterInfo ci;
		ci.u = col * uvSize;
		ci.v = row * uvSize;
		ci.width = uvSize;
		ci.height = uvSize;

		CharInfoMap[static_cast<char>(i)] = ci;
	}
}

const FCharacterInfo& FFont::GetCharInfo(char32_t InCharacter) const
{
	auto it = CharInfoMap.find(InCharacter);
	if (it != CharInfoMap.end())
	{
		return it->second;
	}

	auto fallbackIt = CharInfoMap.find('?');
	if (fallbackIt != CharInfoMap.end())
	{
		return fallbackIt->second;
	}
		
	static const FCharacterInfo defaultInfo{};
	return defaultInfo;
}

void FFont::SetTexture(const TSharedPtr<FTexture>& InTexture)
{
	Texture = InTexture;
}

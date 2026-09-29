#include "FRenderQueue.h"

#include <algorithm>

void FRenderQueue::Sort()
{
	std::sort(primRenderQ.begin(), primRenderQ.end(),
		[](const FDrawCommand& A, const FDrawCommand& B)
		{
			if (A.DepthBucket != B.DepthBucket)
			{
				return A.DepthBucket < B.DepthBucket;
			}

			return A.RenderStateKey < B.RenderStateKey;
			
			// ↓ 정렬 비용 처리후 활성화

			//if (A.RenderStateKey != B.RenderStateKey)
			//{
			//	return A.RenderStateKey < B.RenderStateKey;
			//}
			//
			//return A.Depth < B.Depth;
		}
	);
}

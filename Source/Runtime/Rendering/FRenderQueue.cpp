#include "FRenderQueue.h"

#include <algorithm>

void FRenderQueue::Sort()
{
	std::sort(primRenderQ.begin(), primRenderQ.end(),
		[](const FDrawCommand& A, const FDrawCommand& B)
		{
			//return A.RenderStateKey < B.RenderStateKey;

			if (A.RenderStateKey != B.RenderStateKey)
			{
				return A.RenderStateKey < B.RenderStateKey;
			}

			return A.Depth < B.Depth;

			//if (A.RenderStateKey != B.RenderStateKey)
			//{
			//	return A.RenderStateKey < B.RenderStateKey;
			//}
			//
			//return A.Depth < B.Depth;
		}
	);
}

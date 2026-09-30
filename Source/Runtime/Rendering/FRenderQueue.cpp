#include "FRenderQueue.h"

#include <algorithm>

void FRenderQueue::Sort()
{
	std::sort(primRenderQ.begin(), primRenderQ.end(),
		[](const FDrawCommand& A, const FDrawCommand& B)
		{
			return A.SortKey < B.SortKey;

			//if (A.DepthBucket != B.DepthBucket)
			//{
			//	return A.DepthBucket < B.DepthBucket;
			//}

			//return A.SortKey < B.SortKey;
			
			// ↓ 정렬 비용 처리후 활성화

			//if (A.SortKey != B.SortKey)
			//{
			//	return A.SortKey < B.SortKey;
			//}
			//
			//return A.Depth < B.Depth;
		}
	);
}

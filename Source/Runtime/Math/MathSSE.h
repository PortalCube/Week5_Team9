#pragma once

#include <immintrin.h>
#include <cstdint>
#include <cmath>

#define MS_ALIGN(n) __declspec(align(n))
#define FORCEINLINE __forceinline
#define VectorShuffle(Vec1, Vec2, X, Y, Z, W) _mm_shuffle_ps(Vec1, Vec2, _MM_SHUFFLE(W, Z, Y, X))

struct FMathSSE
{
	using VectorRegister4Float = __m128;

	// 4개의 float 값을 로드하는 함수
	FORCEINLINE static VectorRegister4Float VectorLoadAligned(const float* Ptr)
	{
		return _mm_load_ps(Ptr);
	}

	// 4개의 float 값을 정렬 여부 상관 없이 로드하는 함수
	FORCEINLINE static VectorRegister4Float VectorLoadUnaligned(const float* Ptr)
	{
		return _mm_loadu_ps(Ptr);
	}

	// 계산 결과를 메모리 주소에 기록
	FORCEINLINE static void VectorStoreAligned(const VectorRegister4Float& Vec, float* Ptr)
	{
		_mm_store_ps(Ptr, Vec);
	}

	FORCEINLINE static void VectorStoreUnaligned(const VectorRegister4Float& Vec, float* Ptr)
	{
		_mm_storeu_ps(Ptr, Vec);
	}

	// float 값 1개를 4개 슬롯에 복제하는 함수
	FORCEINLINE static VectorRegister4Float VectorSetFloat1(float Value)
	{
		return _mm_set1_ps(Value);
	}

	FORCEINLINE static VectorRegister4Float VectorAdd(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_add_ps(A, B);
	}

	FORCEINLINE static VectorRegister4Float VectorSub(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_sub_ps(A, B);
	}

	FORCEINLINE static VectorRegister4Float VectorMul(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_mul_ps(A, B);
	}

	FORCEINLINE static VectorRegister4Float VectorDiv(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_div_ps(A, B);
	}

	FORCEINLINE static VectorRegister4Float VectorMulAdd(VectorRegister4Float A, VectorRegister4Float B, VectorRegister4Float C)
	{
#if defined(__AVX2__) || defined(__FMA__)
		return _mm_fmadd_ps(A, B, C);
#else
		return _mm_add_ps(_mm_mul_ps(A, B), C);
#endif
	}

	FORCEINLINE static VectorRegister4Float VectorDot3(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_dp_ps(A, B, 0x7F);
	}

	FORCEINLINE static VectorRegister4Float VectorDot4(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_dp_ps(A, B, 0xFF);
	}

	FORCEINLINE static VectorRegister4Float VectorCross3(VectorRegister4Float A, VectorRegister4Float B)
	{
		VectorRegister4Float A_yzx = VectorShuffle(A, A, 1, 2, 0, 3);
		VectorRegister4Float B_zxy = VectorShuffle(B, B, 2, 0, 1, 3);
		VectorRegister4Float A_zxy = VectorShuffle(A, A, 2, 0, 1, 3);
		VectorRegister4Float B_yzx = VectorShuffle(B, B, 1, 2, 0, 3);

		return _mm_sub_ps(_mm_mul_ps(A_yzx, B_zxy), _mm_mul_ps(A_zxy, B_yzx));
	}

	FORCEINLINE static VectorRegister4Float VectorSqrt(VectorRegister4Float A)
	{
		return _mm_sqrt_ps(A);
	}

	FORCEINLINE static VectorRegister4Float VectorRsqrt(VectorRegister4Float A)
	{
		return _mm_rsqrt_ps(A);
	}

	// Vec에서 특정 인덱스(Index)에 있는 원소를 뽑아 4개 슬롯에 복제하는 함수
	template<uint32_t Index>
	FORCEINLINE static VectorRegister4Float VectorReplicate(VectorRegister4Float Vec)
	{
		static_assert(Index < 4, "Index must be between 0 and 3");
		return _mm_shuffle_ps(Vec, Vec, _MM_SHUFFLE(Index, Index, Index, Index));
	}

	FORCEINLINE static VectorRegister4Float VectorZero()
	{
		return _mm_setzero_ps();
	}

	FORCEINLINE static VectorRegister4Float VectorAnd(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_and_ps(A, B);
	}

	FORCEINLINE static VectorRegister4Float VectorOr(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_or_ps(A, B);
	}

	FORCEINLINE static VectorRegister4Float VectorXor(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_xor_ps(A, B);
	}

	FORCEINLINE static VectorRegister4Float VectorMin(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_min_ps(A, B);
	}

	FORCEINLINE static VectorRegister4Float VectorMax(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_max_ps(A, B);
	}

	// >=, >, <=, < 비교 연산을 수행하는 함수
	FORCEINLINE static VectorRegister4Float VectorCompareGE(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_cmpge_ps(A, B);
	}

	FORCEINLINE static VectorRegister4Float VectorCompareGT(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_cmpgt_ps(A, B);
	}

	FORCEINLINE static VectorRegister4Float VectorCompareLE(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_cmple_ps(A, B);
	}

	FORCEINLINE static VectorRegister4Float VectorCompareLT(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_cmplt_ps(A, B);
	}

	// 4개의 float 슬롯의 최상위 비트만 뽑아 4비트 정수로 변환
	FORCEINLINE static int32_t VectorMaskBits(VectorRegister4Float Vec)
	{
		return _mm_movemask_ps(Vec);
	}

	FORCEINLINE static VectorRegister4Float VectorAndNot(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_andnot_ps(A, B);
	}

	// 최상위 부호 비트를 0으로 설정하고, 마스크와 AND 연산으로 절대값을 계산하는 함수
	FORCEINLINE static VectorRegister4Float VectorAbs(VectorRegister4Float A)
	{
		const __m128 signMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
		return _mm_and_ps(A, signMask);
	}

	// 부호를 반전시키는 함수
	FORCEINLINE static VectorRegister4Float VectorNegate(VectorRegister4Float A)
	{
		const __m128 signMask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
		return _mm_xor_ps(A, signMask);
	}

	// 4개의 float 값을 각각 X, Y, Z, W 슬롯에 설정하는 함수
	FORCEINLINE static VectorRegister4Float VectorSet(float X, float Y, float Z, float W)
	{
		return _mm_set_ps(W, Z, Y, X);
	}

	// Mask ? A : B의 SIMD 버전
	FORCEINLINE static VectorRegister4Float VectorSelect(VectorRegister4Float Mask, VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_blendv_ps(B, A, Mask);
	}
};
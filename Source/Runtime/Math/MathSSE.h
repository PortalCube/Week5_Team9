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

	FORCEINLINE static VectorRegister4Float VectorLoadAligned(const float* Ptr)
	{
		return _mm_load_ps(Ptr);
	}

	FORCEINLINE static VectorRegister4Float VectorLoadUnaligned(const float* Ptr)
	{
		return _mm_loadu_ps(Ptr);
	}

	FORCEINLINE static void VectorStoreAligned(const VectorRegister4Float& Vec, float* Ptr)
	{
		_mm_store_ps(Ptr, Vec);
	}

	FORCEINLINE static void VectorStoreUnaligned(const VectorRegister4Float& Vec, float* Ptr)
	{
		_mm_storeu_ps(Ptr, Vec);
	}

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
		return _mm_fmadd_ps(A, B, C);
	}

	FORCEINLINE static VectorRegister4Float VectorDot3(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_dp_ps(A, B, 0x7F);
	}

	FORCEINLINE static VectorRegister4Float VectorDot4(VectorRegister4Float A, VectorRegister4Float B)
	{
		return _mm_dp_ps(A, B, 0xFF);
	}

	FORCEINLINE static VectorRegister4Float VectorSqrt(VectorRegister4Float A)
	{
		return _mm_sqrt_ps(A);
	}

	FORCEINLINE static VectorRegister4Float VectorRsqrt(VectorRegister4Float A)
	{
		return _mm_rsqrt_ps(A);
	}

	FORCEINLINE static VectorRegister4Float VectorReplicate(VectorRegister4Float Vec, uint32_t Index)
	{
		switch (Index)
		{
		case 0: return _mm_shuffle_ps(Vec, Vec, _MM_SHUFFLE(0, 0, 0, 0));
		case 1: return _mm_shuffle_ps(Vec, Vec, _MM_SHUFFLE(1, 1, 1, 1));
		case 2: return _mm_shuffle_ps(Vec, Vec, _MM_SHUFFLE(2, 2, 2, 2));
		case 3:
		default: return _mm_shuffle_ps(Vec, Vec, _MM_SHUFFLE(3, 3, 3, 3));
		}
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
};
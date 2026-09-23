#pragma once

#include "FVector.h"
#include <numbers>
#include <cmath>

// TODO: 스타일 정리 필요
struct FMatrix
{
	float M[4][4];

	static const FMatrix Identity;

	[[nodiscard]] FMatrix() = default;
	[[nodiscard]] FMatrix(const FVector& InX, const FVector& InY, const FVector& InZ, const FVector& InW);
	[[nodiscard]] explicit FMatrix(float N);

	inline static FMatrix GetIdentity()
	{
		FMatrix t;

		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				t.M[i][j] = (i == j) ? 1.0f : 0.0f;

		return t;
	}

	inline FMatrix Transpose() const
	{
		FMatrix result;

		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				result.M[i][j] = M[j][i];

		return result;
	}

	inline bool Inverse(FMatrix& Dst) const
	{
		const float Det = Determinant();
		if (fabsf(Det) < 1e-8f)
			return false;

		const float rDet = 1.0f / Det;

		FMatrix Result;
		for (int r = 0; r < 4; ++r)
			for (int c = 0; c < 4; ++c)
				Result.M[r][c] = Cofactor(r, c);

		Dst = Result.Transpose() * rDet;
		return true;
	}

	inline float Minor(int r, int c) const
	{
		int R[3], C[3];
		for (int i = 0, k = 0; i < 4; ++i) if (i != r) R[k++] = i;
		for (int j = 0, k = 0; j < 4; ++j) if (j != c) C[k++] = j;

		return
			M[R[0]][C[0]] * (M[R[1]][C[1]] * M[R[2]][C[2]] - M[R[1]][C[2]] * M[R[2]][C[1]]) -
			M[R[0]][C[1]] * (M[R[1]][C[0]] * M[R[2]][C[2]] - M[R[1]][C[2]] * M[R[2]][C[0]]) +
			M[R[0]][C[2]] * (M[R[1]][C[0]] * M[R[2]][C[1]] - M[R[1]][C[1]] * M[R[2]][C[0]]);
	}

	inline float Cofactor(int r, int c) const
	{
		const float Sign = ((r + c) & 1) ? -1.0f : 1.0f;
		return Sign * Minor(r, c);
	}

	//언리얼에서도 그냥 전개식을 때려박음
	inline float Determinant() const
	{
		return	M[0][0] * (
			M[1][1] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) -
			M[2][1] * (M[1][2] * M[3][3] - M[1][3] * M[3][2]) +
			M[3][1] * (M[1][2] * M[2][3] - M[1][3] * M[2][2])
			) -
			M[1][0] * (
				M[0][1] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) -
				M[2][1] * (M[0][2] * M[3][3] - M[0][3] * M[3][2]) +
				M[3][1] * (M[0][2] * M[2][3] - M[0][3] * M[2][2])
				) +
			M[2][0] * (
				M[0][1] * (M[1][2] * M[3][3] - M[1][3] * M[3][2]) -
				M[1][1] * (M[0][2] * M[3][3] - M[0][3] * M[3][2]) +
				M[3][1] * (M[0][2] * M[1][3] - M[0][3] * M[1][2])
				) -
			M[3][0] * (
				M[0][1] * (M[1][2] * M[2][3] - M[1][3] * M[2][2]) -
				M[1][1] * (M[0][2] * M[2][3] - M[0][3] * M[2][2]) +
				M[2][1] * (M[0][2] * M[1][3] - M[0][3] * M[1][2])
				);
	}



	inline static FMatrix MakeScale(const FVector& S)
	{
		FMatrix R = GetIdentity();
		R.M[0][0] = S.X;
		R.M[1][1] = S.Y;
		R.M[2][2] = S.Z;
		return R;
	}

	inline static FMatrix MakeTranslation(const FVector& T)
	{
		FMatrix R = GetIdentity();
		R.M[3][0] = T.X;
		R.M[3][1] = T.Y;
		R.M[3][2] = T.Z;
		return R;
	}

	inline static FMatrix MakeRotationX(float Rad)
	{
		const float c = cosf(Rad), s = sinf(Rad);
		FMatrix R = GetIdentity();
		R.M[1][1] = c;  R.M[1][2] = -s;
		R.M[2][1] = s;  R.M[2][2] = c;
		return R;
	}

	inline static FMatrix MakeRotationY(float Rad)
	{
		const float c = cosf(Rad), s = sinf(Rad);
		FMatrix R = GetIdentity();
		R.M[0][0] = c;  R.M[0][2] = s;
		R.M[2][0] = -s;  R.M[2][2] = c;
		return R;
	}

	inline static FMatrix MakeRotationZ(float Rad)
	{
		const float c = cosf(Rad), s = sinf(Rad);
		FMatrix R = GetIdentity();
		R.M[0][0] = c;  R.M[0][1] = s;
		R.M[1][0] = -s;  R.M[1][1] = c;
		return R;
	}

	inline FMatrix operator*(const FMatrix& Other) const
	{
		FMatrix R;
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				R.M[i][j] = M[i][0] * Other.M[0][j]
				+ M[i][1] * Other.M[1][j]
				+ M[i][2] * Other.M[2][j]
				+ M[i][3] * Other.M[3][j];
		return R;
	}

	inline FMatrix operator*(float Scalar) const
	{
		FMatrix Result;
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				Result.M[i][j] = M[i][j] * Scalar;
		return Result;
	}

	FMatrix& operator*=(const FMatrix& Other)
	{
		*this = *this * Other;
		return *this;
	}

	inline FMatrix ToD3DMatrix()
	{
		static const FMatrix UnrealClipToD3DClip
		{
			FVector{ 0.0f, 0.0f, 1.0f },
			FVector{ 1.0f, 0.0f, 0.0f },
			FVector{ 0.0f, 1.0f, 0.0f },
			FVector{ 0.0f, 0.0f, 0.0f }
		};

		return operator*(UnrealClipToD3DClip);
	}


	[[nodiscard]]
	static FMatrix MakeRotation(const FVector& Deg);

	// Yaw-Pitch-Roll
	[[nodiscard]]
	static FMatrix MakeRotationXYZ(const FVector& Deg);

	// Roll-Pitch-Yaw
	[[nodiscard]]
	static FMatrix MakeRotationZYX(const FVector& Deg);

	[[nodiscard]]
	FVector TransformPointRow(const FVector& p, float w = 1.0f) const;
};

inline const FMatrix FMatrix::Identity = FMatrix{
	FVector(1.0f, 0.0f, 0.0f),
	FVector(0.0f, 1.0f, 0.0f),
	FVector(0.0f, 0.0f, 1.0f),
	FVector(0.0f, 0.0f, 0.0f)
};

inline FMatrix::FMatrix(const FVector& InX, const FVector& InY, const FVector& InZ, const FVector& InW)
{
	M[0][0] = InX.X; M[0][1] = InX.Y; M[0][2] = InX.Z; M[0][3] = 0.0f;
	M[1][0] = InY.X; M[1][1] = InY.Y; M[1][2] = InY.Z; M[1][3] = 0.0f;
	M[2][0] = InZ.X; M[2][1] = InZ.Y; M[2][2] = InZ.Z; M[2][3] = 0.0f;
	M[3][0] = InW.X; M[3][1] = InW.Y; M[3][2] = InW.Z; M[3][3] = 1.0f;
}

inline FMatrix::FMatrix(float N)
{
	for (auto& i : M)
	{
		for (float& j : i)
		{
			j = N;
		}
	}
}

inline FMatrix FMatrix::MakeRotation(const FVector& Deg)
{
	return MakeRotationXYZ(Deg);
}

inline FMatrix FMatrix::MakeRotationXYZ(const FVector& Deg)
{
	constexpr float DegToRad = std::numbers::pi_v<float> / 180.0f;
	return MakeRotationX(Deg.X * DegToRad)
		* MakeRotationY(Deg.Y * DegToRad)
		* MakeRotationZ(Deg.Z * DegToRad);
}

inline FMatrix FMatrix::MakeRotationZYX(const FVector& Deg)
{
	constexpr float DegToRad = std::numbers::pi_v<float> / 180.0f;
	return MakeRotationZ(Deg.Z * DegToRad)
		* MakeRotationY(Deg.Y * DegToRad)
		* MakeRotationX(Deg.X * DegToRad);
}


inline FVector FMatrix::TransformPointRow (const FVector& p, float w) const
{
	float x = p.X * M[0][0] + p.Y * M[1][0] + p.Z * M[2][0] + w * M[3][0];
	float y = p.X * M[0][1] + p.Y * M[1][1] + p.Z * M[2][1] + w * M[3][1];
	float z = p.X * M[0][2] + p.Y * M[1][2] + p.Z * M[2][2] + w * M[3][2];
	float ww = p.X * M[0][3] + p.Y * M[1][3] + p.Z * M[2][3] + w * M[3][3];
	if (ww != 0.0f && ww != 1.0f) { x /= ww; y /= ww; z /= ww; }
	return FVector(x, y, z);
}

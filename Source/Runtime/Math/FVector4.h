#pragma once

#include "Runtime/Core/IntTypes.h"
#include <cassert>
#include <cmath>

#include "FVector.h"

struct FVector4
{
	float X;
	float Y;
	float Z;
	float W;

	static const FVector4 ZeroVector;
	static const FVector4 OneVector;

	[[nodiscard]] constexpr FVector4(float InX = 0.0f, float InY = 0.0f, float InZ = 0.0f, float InW = 0.0f);
	[[nodiscard]] constexpr FVector4(const FVector& V, float InW = 1.0f);

	[[nodiscard]] FVector4 operator+(const FVector4& V) const;

	template<typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	[[nodiscard]] FVector4 operator+(ScalarType Scale) const;

	[[nodiscard]] FVector4 operator-(const FVector4& V) const;

	template<typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	[[nodiscard]] FVector4 operator-(ScalarType Scale) const;

	[[nodiscard]] FVector4 operator*(const FVector4& V) const;

	template <typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	[[nodiscard]] FVector4 operator*(ScalarType Scale) const;

	[[nodiscard]] FVector4 operator/(const FVector4& V) const;

	template <typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	[[nodiscard]] FVector4 operator/(ScalarType Scale) const;

	[[nodiscard]] bool operator==(const FVector4& V) const;

	[[nodiscard]] bool operator!=(const FVector4& V) const;

	[[nodiscard]] FVector4 operator-() const;

	FVector4& operator+=(const FVector4& V);

	template<typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	FVector4& operator+=(ScalarType Scale);

	FVector4& operator-=(const FVector4& V);

	template<typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	FVector4& operator-=(ScalarType Scale);

	FVector4& operator*=(const FVector4& V);

	template<typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	FVector4& operator*=(ScalarType Scale);

	FVector4& operator/=(const FVector4& V);

	template<typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	FVector4& operator/=(ScalarType Scale);

	[[nodiscard]] float& operator[](int32 Index);
	[[nodiscard]] const float& operator[](int32 Index) const;

	[[nodiscard]] float Dot(const FVector4& V) const;

	[[nodiscard]] float Size() const;
	[[nodiscard]] float SizeSquared() const;
};

inline const FVector4 FVector4::ZeroVector{ 0.0f, 0.0f, 0.0f, 0.0f };
inline const FVector4 FVector4::OneVector{ 1.0f, 1.0f, 1.0f, 1.0f };

constexpr FVector4::FVector4(float InX, float InY, float InZ, float InW)
	: X(InX), Y(InY), Z(InZ), W(InW)
{}

constexpr FVector4::FVector4(const FVector& V, float InW)
	: X(V.X), Y(V.Y), Z(V.Z), W(InW)
{}

inline FVector::FVector(const FVector4& V)
	: X(V.X), Y(V.Y), Z(V.Z)
{}

inline FVector4 FVector4::operator+(const FVector4& V) const
{
	return FVector4(X + V.X, Y + V.Y, Z + V.Z, W + V.W);
}

template <typename ScalarType>
	requires std::is_arithmetic_v<ScalarType>
FVector4 FVector4::operator+(ScalarType Scale) const
{
	return FVector4(X + Scale, Y + Scale, Z + Scale, W + Scale);
}

inline FVector4 FVector4::operator-(const FVector4& V) const
{
	return FVector4(X - V.X, Y - V.Y, Z - V.Z, W - V.W);
}

template <typename ScalarType>
	requires std::is_arithmetic_v<ScalarType>
FVector4 FVector4::operator-(ScalarType Scale) const
{
	return FVector4(X - Scale, Y - Scale, Z - Scale, W - Scale);
}

inline FVector4 FVector4::operator*(const FVector4& V) const
{
	return FVector4(X * V.X, Y * V.Y, Z * V.Z, W * V.W);
}

template <typename ScalarType>
	requires std::is_arithmetic_v<ScalarType>
FVector4 FVector4::operator*(ScalarType Scale) const
{
	return FVector4(X * Scale, Y * Scale, Z * Scale, W * Scale);
}

inline FVector4 FVector4::operator/(const FVector4& V) const
{
	return FVector4(X / V.X, Y / V.Y, Z / V.Z, W / V.W);
}

template <typename ScalarType>
	requires std::is_arithmetic_v<ScalarType>
FVector4 FVector4::operator/(ScalarType Scale) const
{
	return FVector4(X / Scale, Y / Scale, Z / Scale, W / Scale);
}

inline bool FVector4::operator==(const FVector4& V) const
{
	return X == V.X && Y == V.Y && Z == V.Z && W == V.W;
}

inline bool FVector4::operator!=(const FVector4& V) const
{
	return !(*this == V);
}

inline FVector4 FVector4::operator-() const
{
	return FVector4(-X, -Y, -Z, -W);
}

inline FVector4& FVector4::operator+=(const FVector4& V)
{
	X += V.X; Y += V.Y; Z += V.Z; W += V.W;
	return *this;
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector4& FVector4::operator+=(ScalarType Scale)
{
	X += Scale; Y += Scale; Z += Scale; W += Scale;
	return *this;
}

inline FVector4& FVector4::operator-=(const FVector4& V)
{
	X -= V.X; Y -= V.Y; Z -= V.Z; W -= V.W;
	return *this;
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector4& FVector4::operator-=(ScalarType Scale)
{
	X -= Scale; Y -= Scale; Z -= Scale; W -= Scale;
	return *this;
}

inline FVector4& FVector4::operator*=(const FVector4& V)
{
	X *= V.X; Y *= V.Y; Z *= V.Z; W *= V.W;
	return *this;
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector4& FVector4::operator*=(ScalarType Scale)
{
	X *= Scale; Y *= Scale; Z *= Scale; W *= Scale;
	return *this;
}

inline FVector4& FVector4::operator/=(const FVector4& V)
{
	X /= V.X; Y /= V.Y; Z /= V.Z; W /= V.W;
	return *this;
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector4& FVector4::operator/=(ScalarType Scale)
{
	X /= Scale; Y /= Scale; Z /= Scale; W /= Scale;
	return *this;
}

inline float& FVector4::operator[](int32 Index)
{
	assert(Index >= 0 && Index < 4);
	return Index == 0 ? X : (Index == 1 ? Y : (Index == 2 ? Z : W));
}

inline const float& FVector4::operator[](int32 Index) const
{
	assert(Index >= 0 && Index < 4);
	return Index == 0 ? X : (Index == 1 ? Y : (Index == 2 ? Z : W));
}

inline float FVector4::Dot(const FVector4& V) const
{
	return X * V.X + Y * V.Y + Z * V.Z + W * V.W;
}

inline float FVector4::Size() const
{
	return std::sqrt(SizeSquared());
}

inline float FVector4::SizeSquared() const
{
	return Dot(*this);
}

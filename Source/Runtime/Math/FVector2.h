#pragma once

#include "Runtime/Core/IntTypes.h"
#include <concepts>
#include <cmath>

struct FVector2
{
	float X;
	float Y;

	static const FVector2 ZeroVector;
	static const FVector2 OneVector;

	[[nodiscard]] constexpr FVector2(float InX = 0.0f, float InY = 0.0f);

	[[nodiscard]] FVector2 operator+(const FVector2& V) const;

	template<typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	[[nodiscard]] FVector2 operator+(ScalarType Scale) const;

	[[nodiscard]] FVector2 operator-(const FVector2& V) const;

	template<typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	[[nodiscard]] FVector2 operator-(ScalarType Scale) const;

	[[nodiscard]] FVector2 operator*(const FVector2& V) const;

	template <typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	[[nodiscard]] FVector2 operator*(ScalarType Scale) const;

	[[nodiscard]] FVector2 operator/(const FVector2& V) const;

	template <typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	[[nodiscard]] FVector2 operator/(ScalarType Scale) const;

	[[nodiscard]] bool operator==(const FVector2& V) const;

	[[nodiscard]] bool operator!=(const FVector2& V) const;

	[[nodiscard]] FVector2 operator-() const;

	FVector2& operator+=(const FVector2& V);

	template<typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	FVector2& operator+=(ScalarType Scale);

	FVector2& operator-=(const FVector2& V);

	template<typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	FVector2& operator-=(ScalarType Scale);

	FVector2& operator*=(const FVector2& V);

	template<typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	FVector2& operator*=(ScalarType Scale);

	FVector2& operator/=(const FVector2& V);

	template<typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	FVector2& operator/=(ScalarType Scale);

	[[nodiscard]] float& operator[](int32 Index);
	[[nodiscard]] const float& operator[](int32 Index) const;

	[[nodiscard]] float Dot(const FVector2& V) const;

	[[nodiscard]] float Size() const;
	[[nodiscard]] float SizeSquared() const;

	[[nodiscard]] FVector2 Normalized() const;
};

inline const FVector2 FVector2::ZeroVector{ 0.0f, 0.0f };
inline const FVector2 FVector2::OneVector{ 1.0f, 1.0f };

constexpr FVector2::FVector2(float InX, float InY)
	: X(InX), Y(InY)
{}

inline FVector2 FVector2::operator+(const FVector2& V) const
{
	return { X + V.X, Y + V.Y };
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector2 FVector2::operator+(ScalarType Scale) const
{
	return { X + Scale, Y + Scale };
}

inline FVector2 FVector2::operator-(const FVector2& V) const
{
	return { X - V.X, Y - V.Y };
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector2 FVector2::operator-(ScalarType Scale) const
{
	return { X - Scale, Y - Scale };
}

inline FVector2 FVector2::operator*(const FVector2& V) const
{
	return { X * V.X, Y * V.Y };
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector2 FVector2::operator*(ScalarType Scale) const
{
	return { X * Scale, Y * Scale };
}

inline FVector2 FVector2::operator/(const FVector2& V) const
{
	return { X / V.X, Y / V.Y };
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector2 FVector2::operator/(ScalarType Scale) const
{
	return { X / Scale, Y / Scale };
}

inline bool FVector2::operator==(const FVector2& V) const
{
	return X == V.X && Y == V.Y;
}

inline bool FVector2::operator!=(const FVector2& V) const
{
	return !(*this == V);
}

inline FVector2 FVector2::operator-() const
{
	return { -X, -Y };
}

inline FVector2& FVector2::operator+=(const FVector2& V)
{
	X += V.X; Y += V.Y;
	return *this;
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector2& FVector2::operator+=(ScalarType Scale)
{
	X += Scale; Y += Scale;
	return *this;
}

inline FVector2& FVector2::operator-=(const FVector2& V)
{
	X -= V.X; Y -= V.Y;
	return *this;
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector2& FVector2::operator-=(ScalarType Scale)
{
	X -= Scale; Y -= Scale;
	return *this;
}

inline FVector2& FVector2::operator*=(const FVector2& V)
{
	X *= V.X; Y *= V.Y;
	return *this;
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector2& FVector2::operator*=(ScalarType Scale)
{
	X *= Scale; Y *= Scale;
	return *this;
}

inline FVector2& FVector2::operator/=(const FVector2& V)
{
	X /= V.X; Y /= V.Y;
	return *this;
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector2& FVector2::operator/=(ScalarType Scale)
{
	X /= Scale; Y /= Scale;
	return *this;
}

inline float& FVector2::operator[](int32 Index)
{
	return *(&X + Index);
}

inline const float& FVector2::operator[](int32 Index) const
{
	return *(&X + Index);
}

inline float FVector2::Dot(const FVector2& V) const
{
	return X * V.X + Y * V.Y;
}

inline float FVector2::Size() const
{
	return std::sqrt(SizeSquared());
}

inline float FVector2::SizeSquared() const
{
	return X * X + Y * Y;
}

inline FVector2 FVector2::Normalized() const
{
	const float Length = Size();
	if (Length == 0.0f)
	{
		return ZeroVector;
	}
	return *this / Length;
}

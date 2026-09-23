#pragma once

#include "Runtime/Core/IntTypes.h"
#include <cassert>
#include <cmath>

struct FVector4;

struct FVector
{
	float X;
	float Y;
	float Z;

	static const FVector ZeroVector;
	static const FVector OneVector;
	static const FVector UpVector;
	static const FVector DownVector;
	static const FVector ForwardVector;
	static const FVector BackwardVector;
	static const FVector RightVector;
	static const FVector LeftVector;

	[[nodiscard]] constexpr FVector(float InX = 0.0f, float InY = 0.0f, float InZ = 0.0f);
	// [[nodiscard]] explicit FVector(const FVector2& V, float InZ);
	[[nodiscard]] FVector(const FVector4& V);

	[[nodiscard]] FVector operator+(const FVector& V) const;

	template<typename ScalarType> 
		requires std::is_arithmetic_v<ScalarType>
	[[nodiscard]] FVector operator+(ScalarType Scale) const;

	[[nodiscard]] FVector operator-(const FVector& V) const;

	template<typename ScalarType> 
		requires std::is_arithmetic_v<ScalarType>
	[[nodiscard]] FVector operator-(ScalarType Scale) const;

	[[nodiscard]] FVector operator*(const FVector& V) const;

	template <typename ScalarType> 
		requires std::is_arithmetic_v<ScalarType>
	[[nodiscard]] FVector operator*(ScalarType Scale) const;

	[[nodiscard]] FVector operator/(const FVector& V) const;

	template <typename ScalarType> 
		requires std::is_arithmetic_v<ScalarType>
	[[nodiscard]] FVector operator/(ScalarType Scale) const;

	[[nodiscard]] bool operator==(const FVector& V) const;

	[[nodiscard]] bool operator!=(const FVector& V) const;

	[[nodiscard]] FVector operator-() const;

	FVector& operator+=(const FVector& V);

	template<typename ScalarType> 
		requires std::is_arithmetic_v<ScalarType>
	FVector& operator+=(ScalarType Scale);

	FVector& operator-=(const FVector& V);

	template<typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	FVector& operator-=(ScalarType Scale);

	FVector& operator*=(const FVector& V);

	template<typename ScalarType> 
		requires std::is_arithmetic_v<ScalarType>
	FVector& operator*=(ScalarType Scale);

	FVector& operator/=(const FVector& V);

	template<typename ScalarType>
		requires std::is_arithmetic_v<ScalarType>
	FVector& operator/=(ScalarType Scale);

	[[nodiscard]] float& operator[](int32 Index);
	[[nodiscard]] const float& operator[](int32 Index) const;

	[[nodiscard]] FVector Cross(const FVector& V) const;

	[[nodiscard]] float Dot(const FVector& V) const;

	[[nodiscard]] float Size() const;
	[[nodiscard]] float SizeSquared() const;

	void Normalize();
};

inline const FVector FVector::ZeroVector{ 0.0f, 0.0f, 0.0f };
inline const FVector FVector::OneVector{ 1.0f, 1.0f, 1.0f };
inline const FVector FVector::UpVector{ 0.0f, 0.0f, 1.0f };
inline const FVector FVector::DownVector{ 0.0f, 0.0f, -1.0f };
inline const FVector FVector::ForwardVector{ 1.0f, 0.0f, 0.0f };
inline const FVector FVector::BackwardVector{ -1.0f, 0.0f, 0.0f };
inline const FVector FVector::RightVector{ 0.0f, 1.0f, 0.0f };
inline const FVector FVector::LeftVector{ 0.0f, -1.0f, 0.0f };

constexpr FVector::FVector(float InX, float InY, float InZ)
	: X(InX), Y(InY), Z(InZ)
{}

inline FVector FVector::operator+(const FVector& V) const
{
	return FVector(X + V.X, Y + V.Y, Z + V.Z);
}

template <typename ScalarType>
	requires std::is_arithmetic_v<ScalarType>
FVector FVector::operator+(ScalarType Scale) const
{
	return FVector(X + Scale, Y + Scale, Z + Scale);
}

inline FVector FVector::operator-(const FVector& V) const
{
	return FVector(X - V.X, Y - V.Y, Z - V.Z);
}

template <typename ScalarType>
	requires std::is_arithmetic_v<ScalarType>
FVector FVector::operator-(ScalarType Scale) const
{
	return FVector(X - Scale, Y - Scale, Z - Scale);
}

inline FVector FVector::operator*(const FVector& V) const
{
	return FVector(X * V.X, Y * V.Y, Z * V.Z);
}

template <typename ScalarType> 
	requires std::is_arithmetic_v<ScalarType>
FVector FVector::operator*(ScalarType Scale) const
{
	return FVector(X * Scale, Y * Scale, Z * Scale);
}

inline FVector FVector::operator/(const FVector& V) const
{
	return FVector(X / V.X, Y / V.Y, Z / V.Z);
}

template <typename ScalarType> 
	requires std::is_arithmetic_v<ScalarType>
FVector FVector::operator/(ScalarType Scale) const
{
	return FVector(X / Scale, Y / Scale, Z / Scale);
}

inline bool FVector::operator==(const FVector& V) const
{
	return X == V.X && Y == V.Y && Z == V.Z;
}

inline bool FVector::operator!=(const FVector& V) const
{
	return !(*this == V);
}

inline FVector FVector::operator-() const
{
	return FVector(-X, -Y, -Z);
}

inline FVector& FVector::operator+=(const FVector& V)
{
	X += V.X; Y += V.Y; Z += V.Z;
	return *this;
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector& FVector::operator+=(ScalarType Scale)
{
	X += Scale; Y += Scale; Z += Scale;
	return *this;
}

inline FVector& FVector::operator-=(const FVector& V)
{
	X -= V.X; Y -= V.Y; Z -= V.Z;
	return *this;
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector& FVector::operator-=(ScalarType Scale)
{
	X -= Scale; Y -= Scale; Z -= Scale;
	return *this;
}

inline FVector& FVector::operator*=(const FVector& V)
{
	X *= V.X; Y *= V.Y; Z *= V.Z;
	return *this;
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector& FVector::operator*=(ScalarType Scale)
{
	X *= Scale; Y *= Scale; Z *= Scale;
	return *this;
}

inline FVector& FVector::operator/=(const FVector& V)
{
	X /= V.X; Y /= V.Y; Z /= V.Z;
	return *this;
}

template <typename ScalarType> requires std::is_arithmetic_v<ScalarType>
FVector& FVector::operator/=(ScalarType Scale)
{
	X /= Scale; Y /= Scale; Z /= Scale;
	return *this;
}

inline float& FVector::operator[](int32 Index)
{
	assert(Index >= 0 && Index < 3);
	return Index == 0 ? X : (Index == 1 ? Y : Z);
}

inline const float& FVector::operator[](int32 Index) const
{
	assert(Index >= 0 && Index < 3);
	return Index == 0 ? X : (Index == 1 ? Y : Z);
}

inline FVector FVector::Cross(const FVector& V) const
{
	return FVector(Y * V.Z - Z * V.Y, Z * V.X - X * V.Z, X * V.Y - Y * V.X);
}

inline float FVector::Dot(const FVector& V) const
{
	return X * V.X + Y * V.Y + Z * V.Z;
}

inline float FVector::Size() const
{
	return std::sqrt(SizeSquared());
}

inline float FVector::SizeSquared() const
{
	return Dot(*this);
}

inline void FVector::Normalize() 
{
	float Length = this->SizeSquared();
	X /= Length;
	Y /= Length;
	Z /= Length;
}
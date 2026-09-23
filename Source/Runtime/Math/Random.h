#pragma once

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <random>

// Source: https://www.learncpp.com/cpp-tutorial/global-random-numbers-random-h/

namespace Random
{
	/** Creates a Mersenne Twister initialized with a non-deterministic seed. */
	inline std::mt19937 Generate()
	{
		std::random_device RandomDevice;
		std::seed_seq SeedSequence
		{
			static_cast<std::seed_seq::result_type>(std::chrono::steady_clock::now().time_since_epoch().count()),
			RandomDevice(),
			RandomDevice(),
			RandomDevice(),
			RandomDevice(),
			RandomDevice(),
			RandomDevice(),
			RandomDevice()
		};

		return std::mt19937{ SeedSequence };
	}

	inline std::mt19937 Generator{ Generate() };

	/** Returns a random integer in the inclusive range [InMin, InMax]. */
	inline int Get(int InMin, int InMax)
	{
		return std::uniform_int_distribution{ InMin, InMax }(Generator);
	}

	/** Returns a random float in the inclusive range at the requested decimal precision. */
	inline float GetFloat(float InMin, float InMax, int InPrecision = 1)
	{
		const std::int64_t Scale = static_cast<std::int64_t>(std::pow(10.0, InPrecision));
		const std::int64_t ScaledMin = static_cast<std::int64_t>(std::ceil(InMin * Scale));
		const std::int64_t ScaledMax = static_cast<std::int64_t>(std::floor(InMax * Scale));

		const std::int64_t ScaledValue = std::uniform_int_distribution<std::int64_t>{ ScaledMin, ScaledMax }(Generator);

		return static_cast<float>(ScaledValue) / static_cast<float>(Scale);
	}

	/** Returns a random integer in the inclusive range [InMin, InMax]. */
	template <typename ValueType>
	ValueType Get(ValueType InMin, ValueType InMax)
	{
		return std::uniform_int_distribution<ValueType>{ InMin, InMax }(Generator);
	}

	/** Returns a random integer after converting both bounds to ReturnType. */
	template <typename ReturnType, typename MinType, typename MaxType>
	ReturnType Get(MinType InMin, MaxType InMax)
	{
		return Get<ReturnType>(static_cast<ReturnType>(InMin), static_cast<ReturnType>(InMax));
	}
}

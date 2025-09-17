#pragma once

#include <cmath>

namespace trnr {

constexpr float A_LAW_A = 87.6f;

inline float alaw_encode(float input)
{
	float sign = (input >= 0.0f) ? 1.0f : -1.0f;
	float abs_sample = std::fabs(input);

	float output;
	if (abs_sample < (1.0f / A_LAW_A)) {
		output = sign * (A_LAW_A * abs_sample) / (1.0f + std::log(A_LAW_A));
	} else {
		output = sign * (1.0f + std::log(A_LAW_A * abs_sample)) / (1.0f + std::log(A_LAW_A));
	}

	return output;
}

inline float alaw_decode(float input)
{
	float sign = (input >= 0.0f) ? 1.0f : -1.0f;
	float abs_comp = std::fabs(input);

	float sample;
	if (abs_comp < (1.0f / (1.0f + std::log(A_LAW_A)))) {
		sample = sign * (abs_comp * (1.0f + std::log(A_LAW_A))) / A_LAW_A;
	} else {
		sample = sign * std::exp(abs_comp * (1.0f + std::log(A_LAW_A)) - 1.0f) / A_LAW_A;
	}

	return sample;
}
} // namespace trnr
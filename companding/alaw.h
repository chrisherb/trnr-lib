/*
 * fold.h
 * Copyright (c) 2025 Christopher Herb
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
		output =
			sign * (1.0f + std::log(A_LAW_A * abs_sample)) / (1.0f + std::log(A_LAW_A));
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

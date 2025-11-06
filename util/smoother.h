/*
 * smoother.h
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

#include "audio_math.h"

namespace trnr {

struct smoother {
	float samplerate;
	float time_samples;

	float current;
	float target;
	float increment;
	int32_t remaining;
};

inline void smoother_init(smoother& s, double samplerate, float time_ms,
						  float initial_value = 0.0f)
{
	s.samplerate = fmax(0.0, samplerate);
	s.time_samples = ms_to_samples(time_ms, s.samplerate);
	s.current = initial_value;
	s.target = initial_value;
	s.increment = 0.0f;
	s.remaining = 0;
}

inline void smoother_set_target(smoother& s, float newTarget)
{
	s.target = newTarget;

	// immediate if time is zero or too short
	if (s.time_samples <= 1.0f) {
		s.current = s.target;
		s.increment = 0.0f;
		s.remaining = 0;
		return;
	}

	int32_t n = static_cast<int32_t>(fmax(1.0f, ceilf(s.time_samples)));
	s.remaining = n;
	// protect against denormals / tiny differences, but we want exact reach at the end
	s.increment = (s.target - s.current) / static_cast<float>(s.remaining);
}

// process a single sample and return the smoothed value
inline float smoother_process_sample(smoother& s)
{
	if (s.remaining > 0) {
		s.current += s.increment;
		--s.remaining;
		if (s.remaining == 0) {
			// ensure exact target at the end to avoid FP drift
			s.current = s.target;
			s.increment = 0.0f;
		}
	}
	return s.current;
}

} // namespace trnr

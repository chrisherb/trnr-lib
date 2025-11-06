/*
 * rms_detector.h
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
struct rms_detector {
	float alpha;
	float rms_squared;
};

inline void rms_init(rms_detector& d, float samplerate, float window_ms)
{
	float window_seconds = 0.001f * window_ms;
	d.alpha = 1.0f - expf(-1.0f / (samplerate * window_seconds));
	d.rms_squared = 0.0f;
}

template <typename sample>
inline sample rms_process(rms_detector& d, sample input)
{
	d.rms_squared = (1.0f - d.alpha) * d.rms_squared + d.alpha * (input * input);
	return sqrtf(d.rms_squared);
}
} // namespace trnr

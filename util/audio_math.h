/*
 * audio_math.h
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

#include <math.h>

namespace trnr {

inline double lin_2_db(double lin)
{
	if (lin <= 1e-20) lin = 1e-20; // avoid log(0)
	return 20.0 * log10(lin);
}

inline double db_2_lin(double db) { return pow(10.0, db / 20.0); }

inline float midi_to_frequency(float midi_note)
{
	return 440.0 * powf(2.0, ((float)midi_note - 69.0) / 12.0);
}

inline float ms_to_samples(float ms, double sample_rate)
{
	return (ms * 0.001f) * (float)sample_rate;
}
} // namespace trnr

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

namespace trnr {
// folds the wave from -1 to 1
inline float fold(float& sample)
{
	while (sample > 1.0 || sample < -1.0) {

		if (sample > 1.0) {
			sample = 2.0 - sample;
		} else if (sample < -1.0) {
			sample = -2.0 - sample;
		}
	}
	return sample;
}

// folds the positive part of the wave independently from the negative part.
inline float fold_bipolar(float& sample)
{
	// fold positive values
	if (sample > 1.0) {
		sample = 2.0 - sample;

		if (sample < 0.0) { sample = -sample; }

		return fold(sample);
	}
	// fold negative values
	else if (sample < -1.0) {
		sample = -2.0 - sample;

		if (sample > 0.0) { sample = -sample; }

		return fold(sample);
	} else {
		return sample;
	}
}
}; // namespace trnr

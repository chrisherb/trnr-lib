/*
 * chebyshev.h
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

#define _USE_MATH_DEFINES
#include <array>
#include <math.h>

namespace trnr {
class chebyshev {
public:
	chebyshev() {}

	chebyshev(double _samplerate, double _frequency) { reset(_samplerate, _frequency); }

	void set_samplerate(double _samplerate) { samplerate = _samplerate; }

	void set_frequency(double _frequency)
	{
		// First calculate the prewarped digital frequency :
		auto K = tanf(M_PI * _frequency / samplerate);

		// Now we calc some Coefficients :
		auto sg = sinh(passband_ripple);
		auto cg = cosh(passband_ripple);
		cg *= cg;

		std::array<double, 4> coeff;
		coeff[0] = 1 / (cg - 0.85355339059327376220042218105097);
		coeff[1] = K * coeff[0] * sg * 1.847759065022573512256366378792;
		coeff[2] = 1 / (cg - 0.14644660940672623779957781894758);
		coeff[3] = K * coeff[2] * sg * 0.76536686473017954345691996806;

		K *= K; // (just to optimize it a little bit)

		// Calculate the first biquad:
		a0 = 1 / (coeff[1] + K + coeff[0]);
		a1 = 2 * (coeff[0] - K) * a0;
		a2 = (coeff[1] - K - coeff[0]) * a0;
		b0 = a0 * K;
		b1 = 2 * b0;
		b2 = b0;

		// Calculate the second biquad:
		a3 = 1 / (coeff[3] + K + coeff[2]);
		a4 = 2 * (coeff[2] - K) * a3;
		a5 = (coeff[3] - K - coeff[2]) * a3;
		b3 = a3 * K;
		b4 = 2 * b3;
		b5 = b3;
	}

	void reset(double _samplerate, double _frequency)
	{
		set_samplerate(_samplerate);
		set_frequency(_frequency);
	}

	template <typename t_sample>
	void process_sample(t_sample& input)
	{
		auto Stage1 = b0 * input + state0;
		state0 = b1 * input + a1 * Stage1 + state1;
		state1 = b2 * input + a2 * Stage1;
		input = b3 * Stage1 + state2;
		state2 = b4 * Stage1 + a4 * input + state3;
		state3 = b5 * Stage1 + a5 * input;
	}

	template <typename t_sample>
	void process_sample(t_sample& input, double frequency)
	{
		set_frequency(frequency);

		process_sample(input);
	}

	template <typename t_sample>
	void process_block(t_sample* input, int blockSize)
	{
		t_sample output = 0;

		for (int i = 0; i < blockSize; i++) {
			output = input[i];
			process_sample(output);
			input[i] = output;
		}
	}

private:
	double samplerate = 20000;
	double a0 = 0;
	double a1 = 0;
	double a2 = 0;
	double a3 = 0;
	double a4 = 0;
	double a5 = 0;
	double b0 = 0;
	double b1 = 0;
	double b2 = 0;
	double b3 = 0;
	double b4 = 0;
	double b5 = 0;
	double state0 = 0;
	double state1 = 0;
	double state2 = 0;
	double state3 = 0;
	double passband_ripple = 1;
};
} // namespace trnr

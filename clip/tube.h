/*
 * tube.h
 * Copyright (c) 2016 Chris Johnson
 * Copyright (c) 2025 Christopher Herb
 * Based on Tube2 by Chris Johnson, 2016
 * This file is a derivative/major refactor of the above module.
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
 *
 * Changes:
 * - 2025-11-06 Christopher Herb:
 *  - Templated audio buffer i/o
 *  - Converted to procedural programming style
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdint.h>

using namespace std;

namespace trnr {

struct tube {
	double samplerate;

	double prev_sample_a;
	double prev_sample_b;
	double prev_sample_c;
	double prev_sample_d;
	double prev_sample_e;
	double prev_sample_f;

	uint32_t fdp_l;
	uint32_t fdp_r;

	float input_vol;
	float tube_amt;

	void set_input(double value) { input_vol = clamp(value, 0.0, 1.0); }

	void set_tube(double value) { tube_amt = clamp(value, 0.0, 1.0); }
};

inline void tube_init(tube& t, double samplerate)
{
	t.samplerate = 44100;

	t.input_vol = 0.5;
	t.tube_amt = 0.5;
	t.prev_sample_a = 0.0;
	t.prev_sample_b = 0.0;
	t.prev_sample_c = 0.0;
	t.prev_sample_d = 0.0;
	t.prev_sample_e = 0.0;
	t.prev_sample_f = 0.0;
	t.fdp_l = 1.0;
	while (t.fdp_l < 16386) t.fdp_l = rand() * UINT32_MAX;
	t.fdp_r = 1.0;
	while (t.fdp_r < 16386) t.fdp_r = rand() * UINT32_MAX;
}

template <typename t_sample>
inline void tube_process_block(tube& t, t_sample** inputs, t_sample** outputs,
							   long sampleframes)
{
	t_sample* in1 = inputs[0];
	t_sample* in2 = inputs[1];
	t_sample* out1 = outputs[0];
	t_sample* out2 = outputs[1];

	double overallscale = 1.0;
	overallscale /= 44100.0;
	overallscale *= t.samplerate;

	double input_pad = t.input_vol;
	double iterations = 1.0 - t.tube_amt;
	int powerfactor = (9.0 * iterations) + 1;
	double asym_pad = (double)powerfactor;
	double gainscaling = 1.0 / (double)(powerfactor + 1);
	double outputscaling = 1.0 + (1.0 / (double)(powerfactor));

	while (--sampleframes >= 0) {
		double input_l = *in1;
		double input_r = *in2;
		if (fabs(input_l) < 1.18e-23) input_l = t.fdp_l * 1.18e-17;
		if (fabs(input_r) < 1.18e-23) input_r = t.fdp_r * 1.18e-17;

		if (input_pad < 1.0) {
			input_l *= input_pad;
			input_r *= input_pad;
		}

		if (overallscale > 1.9) {
			double stored = input_l;
			input_l += t.prev_sample_a;
			t.prev_sample_a = stored;
			input_l *= 0.5;
			stored = input_r;
			input_r += t.prev_sample_b;
			t.prev_sample_b = stored;
			input_r *= 0.5;
		} // for high sample rates on this plugin we are going to do a simple average

		if (input_l > 1.0) input_l = 1.0;
		if (input_l < -1.0) input_l = -1.0;
		if (input_r > 1.0) input_r = 1.0;
		if (input_r < -1.0) input_r = -1.0;

		// flatten bottom, point top of sine waveshaper L
		input_l /= asym_pad;
		double sharpen = -input_l;
		if (sharpen > 0.0) sharpen = 1.0 + sqrt(sharpen);
		else sharpen = 1.0 - sqrt(-sharpen);
		input_l -= input_l * fabs(input_l) * sharpen * 0.25;
		// this will take input from exactly -1.0 to 1.0 max
		input_l *= asym_pad;
		// flatten bottom, point top of sine waveshaper R
		input_r /= asym_pad;
		sharpen = -input_r;
		if (sharpen > 0.0) sharpen = 1.0 + sqrt(sharpen);
		else sharpen = 1.0 - sqrt(-sharpen);
		input_r -= input_r * fabs(input_r) * sharpen * 0.25;
		// this will take input from exactly -1.0 to 1.0 max
		input_r *= asym_pad;
		// end first asym section: later boosting can mitigate the extreme
		// softclipping of one side of the wave
		// and we are asym clipping more when Tube is cranked, to compensate

		// original Tube algorithm: powerfactor widens the more linear region of the wave
		double factor = input_l; // Left channel
		for (int x = 0; x < powerfactor; x++) factor *= input_l;
		if ((powerfactor % 2 == 1) && (input_l != 0.0))
			factor = (factor / input_l) * fabs(input_l);
		factor *= gainscaling;
		input_l -= factor;
		input_l *= outputscaling;
		factor = input_r; // Right channel
		for (int x = 0; x < powerfactor; x++) factor *= input_r;
		if ((powerfactor % 2 == 1) && (input_r != 0.0))
			factor = (factor / input_r) * fabs(input_r);
		factor *= gainscaling;
		input_r -= factor;
		input_r *= outputscaling;

		if (overallscale > 1.9) {
			double stored = input_l;
			input_l += t.prev_sample_c;
			t.prev_sample_c = stored;
			input_l *= 0.5;
			stored = input_r;
			input_r += t.prev_sample_d;
			t.prev_sample_d = stored;
			input_r *= 0.5;
		} // for high sample rates on this plugin we are going to do a simple average
		// end original Tube. Now we have a boosted fat sound peaking at 0dB exactly

		// hysteresis and spiky fuzz L
		double slew = t.prev_sample_e - input_l;
		if (overallscale > 1.9) {
			double stored = input_l;
			input_l += t.prev_sample_e;
			t.prev_sample_e = stored;
			input_l *= 0.5;
		} else t.prev_sample_e = input_l; // for this, need previousSampleC always
		if (slew > 0.0) slew = 1.0 + (sqrt(slew) * 0.5);
		else slew = 1.0 - (sqrt(-slew) * 0.5);
		input_l -= input_l * fabs(input_l) * slew * gainscaling;
		// reusing gainscaling that's part of another algorithm
		if (input_l > 0.52) input_l = 0.52;
		if (input_l < -0.52) input_l = -0.52;
		input_l *= 1.923076923076923;
		// hysteresis and spiky fuzz R
		slew = t.prev_sample_f - input_r;
		if (overallscale > 1.9) {
			double stored = input_r;
			input_r += t.prev_sample_f;
			t.prev_sample_f = stored;
			input_r *= 0.5;
		} else t.prev_sample_f = input_r; // for this, need previousSampleC always
		if (slew > 0.0) slew = 1.0 + (sqrt(slew) * 0.5);
		else slew = 1.0 - (sqrt(-slew) * 0.5);
		input_r -= input_r * fabs(input_r) * slew * gainscaling;
		// reusing gainscaling that's part of another algorithm
		if (input_r > 0.52) input_r = 0.52;
		if (input_r < -0.52) input_r = -0.52;
		input_r *= 1.923076923076923;
		// end hysteresis and spiky fuzz section

		// begin 64 bit stereo floating point dither
		// int expon; frexp((double)inputSampleL, &expon);
		t.fdp_l ^= t.fdp_l << 13;
		t.fdp_l ^= t.fdp_l >> 17;
		t.fdp_l ^= t.fdp_l << 5;
		// inputSampleL += ((double(fpdL)-uint32_t(0x7fffffff)) * 1.1e-44l *
		// pow(2,expon+62)); frexp((double)inputSampleR, &expon);
		t.fdp_r ^= t.fdp_r << 13;
		t.fdp_r ^= t.fdp_r >> 17;
		t.fdp_r ^= t.fdp_r << 5;
		// inputSampleR += ((double(fpdR)-uint32_t(0x7fffffff)) * 1.1e-44l *
		// pow(2,expon+62)); end 64 bit stereo floating point dither

		*out1 = input_l;
		*out2 = input_r;

		in1++;
		in2++;
		out1++;
		out2++;
	}
}
} // namespace trnr

#pragma once

#define _USE_MATH_DEFINES
#include <array>
#include <math.h>
#include <stdint.h>

using namespace std;

namespace trnr {

////////////
// COMMON //
////////////

enum {
	Y_BIQ_FREQ,
	Y_BIQ_RESO,
	Y_BIQ_A0,
	Y_BIQ_A1,
	Y_BIQ_A2,
	Y_BIQ_B1,
	Y_BIQ_B2,
	Y_BIQ_A_A0,
	Y_BIQ_A_A1,
	Y_BIQ_A_A2,
	Y_BIQ_B_A1,
	Y_BIQ_B_A2,
	Y_BIQ_A_B0,
	Y_BIQ_A_B1,
	Y_BIQ_A_B2,
	Y_BIQ_B_B1,
	Y_BIQ_B_B2,
	Y_BIQ_S_L1,
	Y_BIQ_S_L2,
	Y_BIQ_S_R1,
	Y_BIQ_S_R2,
	Y_BIQ_TOTAL
}; // coefnncient interpolating biquad filter, stereo

enum {
	Y_FIX_FREQ,
	Y_FIX_RESO,
	Y_FIX_A0,
	Y_FIX_A1,
	Y_FIX_A2,
	Y_FIX_B1,
	Y_FIX_B2,
	Y_FIX_S_L1,
	Y_FIX_S_L2,
	Y_FIX_S_R1,
	Y_FIX_S_R2,
	Y_FIX_TOTAL
}; // fixed frequency biquad filter for ultrasonics, stereo

/////////////
// LOWPASS //
/////////////

struct ylowpass {
	double samplerate;
	array<double, Y_BIQ_TOTAL> biquad;

	double powFactorA;
	double powFactorB;
	double inTrimA;
	double inTrimB;
	double outTrimA;
	double outTrimB;

	array<double, Y_FIX_TOTAL> fixA;
	array<double, Y_FIX_TOTAL> fixB;

	uint32_t fpdL;
	uint32_t fpdR;
	// default stuff

	float drive;
	float frequency;
	float resonance;
	float edge;
	float output;
	float mix; // parameters. Always 0-1, and we scale/alter them elsewhere.
};

inline void ylowpass_init(ylowpass& y, double samplerate)
{
	y.samplerate = samplerate;
	y.drive = 0.1f;
	y.frequency = 1.0f;
	y.resonance = 0.0f;
	y.edge = 0.1f;
	y.output = 0.9f;
	y.mix = 1.0f;

	for (int x = 0; x < Y_BIQ_TOTAL; x++) { y.biquad[x] = 0.0; }
	y.powFactorA = 1.0;
	y.powFactorB = 1.0;
	y.inTrimA = 0.1;
	y.inTrimB = 0.1;
	y.outTrimA = 1.0;
	y.outTrimB = 1.0;
	for (int x = 0; x < Y_FIX_TOTAL; x++) {
		y.fixA[x] = 0.0;
		y.fixB[x] = 0.0;
	}

	y.fpdL = 1.0;
	while (y.fpdL < 16386) y.fpdL = rand() * UINT32_MAX;
	y.fpdR = 1.0;
	while (y.fpdR < 16386) y.fpdR = rand() * UINT32_MAX;
}

template <typename t_sample>
inline void ylowpass_process_block(ylowpass& y, t_sample** inputs, t_sample** outputs,
								   int blockSize)
{
	t_sample* in1 = inputs[0];
	t_sample* in2 = inputs[1];
	t_sample* out1 = outputs[0];
	t_sample* out2 = outputs[1];

	int inFramesToProcess = blockSize;
	double overallscale = 1.0;
	overallscale /= 44100.0;
	overallscale *= y.samplerate;

	y.inTrimA = y.inTrimB;
	y.inTrimB = y.drive * 10.0;

	y.biquad[Y_BIQ_FREQ] = pow(y.frequency, 3) * 20000.0;
	if (y.biquad[Y_BIQ_FREQ] < 15.0) y.biquad[Y_BIQ_FREQ] = 15.0;
	y.biquad[Y_BIQ_FREQ] /= y.samplerate;
	y.biquad[Y_BIQ_RESO] = (pow(y.resonance, 2) * 15.0) + 0.5571;
	y.biquad[Y_BIQ_A_A0] = y.biquad[Y_BIQ_A_B0];
	y.biquad[Y_BIQ_A_A1] = y.biquad[Y_BIQ_A_B1];
	y.biquad[Y_BIQ_A_A2] = y.biquad[Y_BIQ_A_B2];
	y.biquad[Y_BIQ_B_A1] = y.biquad[Y_BIQ_B_B1];
	y.biquad[Y_BIQ_B_A2] = y.biquad[Y_BIQ_B_B2];
	// previous run through the buffer is still in the filter, so we move it
	// to the A section and now it's the new starting point.
	double K = tan(M_PI * y.biquad[Y_BIQ_FREQ]);
	double norm = 1.0 / (1.0 + K / y.biquad[Y_BIQ_RESO] + K * K);
	y.biquad[Y_BIQ_A_B0] = K * K * norm;
	y.biquad[Y_BIQ_A_B1] = 2.0 * y.biquad[Y_BIQ_A_B0];
	y.biquad[Y_BIQ_A_B2] = y.biquad[Y_BIQ_A_B0];
	y.biquad[Y_BIQ_B_B1] = 2.0 * (K * K - 1.0) * norm;
	y.biquad[Y_BIQ_B_B2] = (1.0 - K / y.biquad[Y_BIQ_RESO] + K * K) * norm;
	// for the coefficient-interpolated biquad filter

	y.powFactorA = y.powFactorB;
	y.powFactorB = pow(y.edge + 0.9, 4);

	// 1.0 == target neutral

	y.outTrimA = y.outTrimB;
	y.outTrimB = y.output;

	double wet = y.mix;

	y.fixA[Y_FIX_FREQ] = y.fixB[Y_FIX_FREQ] = 20000.0 / y.samplerate;
	y.fixA[Y_FIX_RESO] = y.fixB[Y_FIX_RESO] = 0.7071; // butterworth Q

	K = tan(M_PI * y.fixA[Y_FIX_FREQ]);
	norm = 1.0 / (1.0 + K / y.fixA[Y_FIX_RESO] + K * K);
	y.fixA[Y_FIX_A0] = y.fixB[Y_FIX_A0] = K * K * norm;
	y.fixA[Y_FIX_A1] = y.fixB[Y_FIX_A1] = 2.0 * y.fixA[Y_FIX_A0];
	y.fixA[Y_FIX_A2] = y.fixB[Y_FIX_A2] = y.fixA[Y_FIX_A0];
	y.fixA[Y_FIX_B1] = y.fixB[Y_FIX_B1] = 2.0 * (K * K - 1.0) * norm;
	y.fixA[Y_FIX_B2] = y.fixB[Y_FIX_B2] = (1.0 - K / y.fixA[Y_FIX_RESO] + K * K) * norm;
	// for the fixed-position biquad filter

	for (int s = 0; s < blockSize; s++) {
		double inputSampleL = *in1;
		double inputSampleR = *in2;
		if (fabs(inputSampleL) < 1.18e-23) inputSampleL = y.fpdL * 1.18e-17;
		if (fabs(inputSampleR) < 1.18e-23) inputSampleR = y.fpdR * 1.18e-17;
		double drySampleL = inputSampleL;
		double drySampleR = inputSampleR;

		double temp = (double)s / inFramesToProcess;
		y.biquad[Y_BIQ_A0] =
			(y.biquad[Y_BIQ_A_A0] * temp) + (y.biquad[Y_BIQ_A_B0] * (1.0 - temp));
		y.biquad[Y_BIQ_A1] =
			(y.biquad[Y_BIQ_A_A1] * temp) + (y.biquad[Y_BIQ_A_B1] * (1.0 - temp));
		y.biquad[Y_BIQ_A2] =
			(y.biquad[Y_BIQ_A_A2] * temp) + (y.biquad[Y_BIQ_A_B2] * (1.0 - temp));
		y.biquad[Y_BIQ_B1] =
			(y.biquad[Y_BIQ_B_A1] * temp) + (y.biquad[Y_BIQ_B_B1] * (1.0 - temp));
		y.biquad[Y_BIQ_B2] =
			(y.biquad[Y_BIQ_B_A2] * temp) + (y.biquad[Y_BIQ_B_B2] * (1.0 - temp));
		// this is the interpolation code for the biquad
		double powFactor = (y.powFactorA * temp) + (y.powFactorB * (1.0 - temp));
		double inTrim = (y.inTrimA * temp) + (y.inTrimB * (1.0 - temp));
		double outTrim = (y.outTrimA * temp) + (y.outTrimB * (1.0 - temp));

		inputSampleL *= inTrim;
		inputSampleR *= inTrim;

		temp = (inputSampleL * y.fixA[Y_FIX_A0]) + y.fixA[Y_FIX_S_L1];
		y.fixA[Y_FIX_S_L1] = (inputSampleL * y.fixA[Y_FIX_A1]) -
							 (temp * y.fixA[Y_FIX_B1]) + y.fixA[Y_FIX_S_L2];
		y.fixA[Y_FIX_S_L2] =
			(inputSampleL * y.fixA[Y_FIX_A2]) - (temp * y.fixA[Y_FIX_B2]);
		inputSampleL = temp; // fixed biquad filtering ultrasonics
		temp = (inputSampleR * y.fixA[Y_FIX_A0]) + y.fixA[Y_FIX_S_R1];
		y.fixA[Y_FIX_S_R1] = (inputSampleR * y.fixA[Y_FIX_A1]) -
							 (temp * y.fixA[Y_FIX_B1]) + y.fixA[Y_FIX_S_R2];
		y.fixA[Y_FIX_S_R2] =
			(inputSampleR * y.fixA[Y_FIX_A2]) - (temp * y.fixA[Y_FIX_B2]);
		inputSampleR = temp; // fixed biquad filtering ultrasonics

		// encode/decode courtesy of torridgristle under the MIT license
		if (inputSampleL > 1.0) inputSampleL = 1.0;
		else if (inputSampleL > 0.0)
			inputSampleL = 1.0 - pow(1.0 - inputSampleL, powFactor);
		if (inputSampleL < -1.0) inputSampleL = -1.0;
		else if (inputSampleL < 0.0)
			inputSampleL = -1.0 + pow(1.0 + inputSampleL, powFactor);
		if (inputSampleR > 1.0) inputSampleR = 1.0;
		else if (inputSampleR > 0.0)
			inputSampleR = 1.0 - pow(1.0 - inputSampleR, powFactor);
		if (inputSampleR < -1.0) inputSampleR = -1.0;
		else if (inputSampleR < 0.0)
			inputSampleR = -1.0 + pow(1.0 + inputSampleR, powFactor);

		temp = (inputSampleL * y.biquad[Y_BIQ_A0]) + y.biquad[Y_BIQ_S_L1];
		y.biquad[Y_BIQ_S_L1] = (inputSampleL * y.biquad[Y_BIQ_A1]) -
							   (temp * y.biquad[Y_BIQ_B1]) + y.biquad[Y_BIQ_S_L2];
		y.biquad[Y_BIQ_S_L2] =
			(inputSampleL * y.biquad[Y_BIQ_A2]) - (temp * y.biquad[Y_BIQ_B2]);
		inputSampleL = temp; // coefficient interpolating biquad filter
		temp = (inputSampleR * y.biquad[Y_BIQ_A0]) + y.biquad[Y_BIQ_S_R1];
		y.biquad[Y_BIQ_S_R1] = (inputSampleR * y.biquad[Y_BIQ_A1]) -
							   (temp * y.biquad[Y_BIQ_B1]) + y.biquad[Y_BIQ_S_R2];
		y.biquad[Y_BIQ_S_R2] =
			(inputSampleR * y.biquad[Y_BIQ_A2]) - (temp * y.biquad[Y_BIQ_B2]);
		inputSampleR = temp; // coefficient interpolating biquad filter

		// encode/decode courtesy of torridgristle under the MIT license
		if (inputSampleL > 1.0) inputSampleL = 1.0;
		else if (inputSampleL > 0.0)
			inputSampleL = 1.0 - pow(1.0 - inputSampleL, (1.0 / powFactor));
		if (inputSampleL < -1.0) inputSampleL = -1.0;
		else if (inputSampleL < 0.0)
			inputSampleL = -1.0 + pow(1.0 + inputSampleL, (1.0 / powFactor));
		if (inputSampleR > 1.0) inputSampleR = 1.0;
		else if (inputSampleR > 0.0)
			inputSampleR = 1.0 - pow(1.0 - inputSampleR, (1.0 / powFactor));
		if (inputSampleR < -1.0) inputSampleR = -1.0;
		else if (inputSampleR < 0.0)
			inputSampleR = -1.0 + pow(1.0 + inputSampleR, (1.0 / powFactor));

		inputSampleL *= outTrim;
		inputSampleR *= outTrim;

		temp = (inputSampleL * y.fixB[Y_FIX_A0]) + y.fixB[Y_FIX_S_L1];
		y.fixB[Y_FIX_S_L1] = (inputSampleL * y.fixB[Y_FIX_A1]) -
							 (temp * y.fixB[Y_FIX_B1]) + y.fixB[Y_FIX_S_L2];
		y.fixB[Y_FIX_S_L2] =
			(inputSampleL * y.fixB[Y_FIX_A2]) - (temp * y.fixB[Y_FIX_B2]);
		inputSampleL = temp; // fixed biquad filtering ultrasonics
		temp = (inputSampleR * y.fixB[Y_FIX_A0]) + y.fixB[Y_FIX_S_R1];
		y.fixB[Y_FIX_S_R1] = (inputSampleR * y.fixB[Y_FIX_A1]) -
							 (temp * y.fixB[Y_FIX_B1]) + y.fixB[Y_FIX_S_R2];
		y.fixB[Y_FIX_S_R2] =
			(inputSampleR * y.fixB[Y_FIX_A2]) - (temp * y.fixB[Y_FIX_B2]);
		inputSampleR = temp; // fixed biquad filtering ultrasonics

		if (wet < 1.0) {
			inputSampleL = (inputSampleL * wet) + (drySampleL * (1.0 - wet));
			inputSampleR = (inputSampleR * wet) + (drySampleR * (1.0 - wet));
		}

		// begin 32 bit stereo floating point dither
		int expon;
		frexpf((float)inputSampleL, &expon);
		y.fpdL ^= y.fpdL << 13;
		y.fpdL ^= y.fpdL >> 17;
		y.fpdL ^= y.fpdL << 5;
		inputSampleL +=
			((double(y.fpdL) - uint32_t(0x7fffffff)) * 5.5e-36l * pow(2, expon + 62));
		frexpf((float)inputSampleR, &expon);
		y.fpdR ^= y.fpdR << 13;
		y.fpdR ^= y.fpdR >> 17;
		y.fpdR ^= y.fpdR << 5;
		inputSampleR +=
			((double(y.fpdR) - uint32_t(0x7fffffff)) * 5.5e-36l * pow(2, expon + 62));
		// end 32 bit stereo floating point dither

		*out1 = inputSampleL;
		*out2 = inputSampleR;

		in1++;
		in2++;
		out1++;
		out2++;
	}
}

//////////////
// HIGHPASS //
//////////////

struct yhighpass {
	double samplerate;
	array<double, Y_BIQ_TOTAL> biquad;

	double powFactorA;
	double powFactorB;
	double inTrimA;
	double inTrimB;
	double outTrimA;
	double outTrimB;

	array<double, Y_FIX_TOTAL> fixA;
	array<double, Y_FIX_TOTAL> fixB;

	uint32_t fpdL;
	uint32_t fpdR;
	// default stuff

	float drive;
	float frequency;
	float resonance;
	float edge;
	float output;
	float mix; // parameters. Always 0-1, and we scale/alter them elsewhere.
};

inline void yhighpass_init(yhighpass& y, double samplerate)
{
	y.samplerate = samplerate;
	y.drive = 0.1f;
	y.frequency = 1.0f;
	y.resonance = 0.0f;
	y.edge = 0.1f;
	y.output = 0.9f;
	y.mix = 1.0f;
	y.fpdL = 0;
	y.fpdR = 0;
	y.biquad.fill(0);

	y.powFactorA = 1.0;
	y.powFactorB = 1.0;
	y.inTrimA = 0.1;
	y.inTrimB = 0.1;
	y.outTrimA = 1.0;
	y.outTrimB = 1.0;
	for (int x = 0; x < Y_FIX_TOTAL; x++) {
		y.fixA[x] = 0.0;
		y.fixB[x] = 0.0;
	}

	y.fpdL = 1.0;
	while (y.fpdL < 16386) y.fpdL = rand() * UINT32_MAX;
	y.fpdR = 1.0;
	while (y.fpdR < 16386) y.fpdR = rand() * UINT32_MAX;
}

template <typename t_sample>
inline void yhighpass_process_block(yhighpass& y, t_sample** inputs, t_sample** outputs,
									int blockSize)
{
	t_sample* in1 = inputs[0];
	t_sample* in2 = inputs[1];
	t_sample* out1 = outputs[0];
	t_sample* out2 = outputs[1];

	int inFramesToProcess = blockSize;
	double overallscale = 1.0;
	overallscale /= 44100.0;
	overallscale *= y.samplerate;

	y.inTrimA = y.inTrimB;
	y.inTrimB = y.drive * 10.0;

	y.biquad[Y_BIQ_FREQ] = pow(y.frequency, 3) * 20000.0;
	if (y.biquad[Y_BIQ_FREQ] < 15.0) y.biquad[Y_BIQ_FREQ] = 15.0;
	y.biquad[Y_BIQ_FREQ] /= y.samplerate;
	y.biquad[Y_BIQ_RESO] = (pow(y.resonance, 2) * 15.0) + 0.5571;
	y.biquad[Y_BIQ_A_A0] = y.biquad[Y_BIQ_A_B0];
	y.biquad[Y_BIQ_A_A1] = y.biquad[Y_BIQ_A_B1];
	y.biquad[Y_BIQ_A_A2] = y.biquad[Y_BIQ_A_B2];
	y.biquad[Y_BIQ_B_A1] = y.biquad[Y_BIQ_B_B1];
	y.biquad[Y_BIQ_B_A2] = y.biquad[Y_BIQ_B_B2];
	// previous run through the buffer is still in the filter, so we move it
	// to the A section and now it's the new starting point.
	double K = tan(M_PI * y.biquad[Y_BIQ_FREQ]);
	double norm = 1.0 / (1.0 + K / y.biquad[Y_BIQ_RESO] + K * K);
	y.biquad[Y_BIQ_A_B0] = norm;
	y.biquad[Y_BIQ_A_B1] = -2.0 * y.biquad[Y_BIQ_A_B0];
	y.biquad[Y_BIQ_A_B2] = y.biquad[Y_BIQ_A_B0];
	y.biquad[Y_BIQ_B_B1] = 2.0 * (K * K - 1.0) * norm;
	y.biquad[Y_BIQ_B_B2] = (1.0 - K / y.biquad[Y_BIQ_RESO] + K * K) * norm;
	// for the coefficient-interpolated biquad filter

	y.powFactorA = y.powFactorB;
	y.powFactorB = pow(y.edge + 0.9, 4);

	// 1.0 == target neutral

	y.outTrimA = y.outTrimB;
	y.outTrimB = y.output;

	double wet = y.mix;

	y.fixA[Y_FIX_FREQ] = y.fixB[Y_FIX_FREQ] = 20000.0 / y.samplerate;
	y.fixA[Y_FIX_RESO] = y.fixB[Y_FIX_RESO] = 0.7071; // butterworth Q

	K = tan(M_PI * y.fixA[Y_FIX_FREQ]);
	norm = 1.0 / (1.0 + K / y.fixA[Y_FIX_RESO] + K * K);
	y.fixA[Y_FIX_A0] = y.fixB[Y_FIX_A0] = K * K * norm;
	y.fixA[Y_FIX_A1] = y.fixB[Y_FIX_A1] = 2.0 * y.fixA[Y_FIX_A0];
	y.fixA[Y_FIX_A2] = y.fixB[Y_FIX_A2] = y.fixA[Y_FIX_A0];
	y.fixA[Y_FIX_B1] = y.fixB[Y_FIX_B1] = 2.0 * (K * K - 1.0) * norm;
	y.fixA[Y_FIX_B2] = y.fixB[Y_FIX_B2] = (1.0 - K / y.fixA[Y_FIX_RESO] + K * K) * norm;
	// for the fixed-position biquad filter

	for (int s = 0; s < blockSize; s++) {
		double inputSampleL = *in1;
		double inputSampleR = *in2;
		if (fabs(inputSampleL) < 1.18e-23) inputSampleL = y.fpdL * 1.18e-17;
		if (fabs(inputSampleR) < 1.18e-23) inputSampleR = y.fpdR * 1.18e-17;
		double drySampleL = inputSampleL;
		double drySampleR = inputSampleR;

		double temp = (double)s / inFramesToProcess;
		y.biquad[Y_BIQ_A0] =
			(y.biquad[Y_BIQ_A_A0] * temp) + (y.biquad[Y_BIQ_A_B0] * (1.0 - temp));
		y.biquad[Y_BIQ_A1] =
			(y.biquad[Y_BIQ_A_A1] * temp) + (y.biquad[Y_BIQ_A_B1] * (1.0 - temp));
		y.biquad[Y_BIQ_A2] =
			(y.biquad[Y_BIQ_A_A2] * temp) + (y.biquad[Y_BIQ_A_B2] * (1.0 - temp));
		y.biquad[Y_BIQ_B1] =
			(y.biquad[Y_BIQ_B_A1] * temp) + (y.biquad[Y_BIQ_B_B1] * (1.0 - temp));
		y.biquad[Y_BIQ_B2] =
			(y.biquad[Y_BIQ_B_A2] * temp) + (y.biquad[Y_BIQ_B_B2] * (1.0 - temp));
		// this is the interpolation code for the biquad
		double powFactor = (y.powFactorA * temp) + (y.powFactorB * (1.0 - temp));
		double inTrim = (y.inTrimA * temp) + (y.inTrimB * (1.0 - temp));
		double outTrim = (y.outTrimA * temp) + (y.outTrimB * (1.0 - temp));

		inputSampleL *= inTrim;
		inputSampleR *= inTrim;

		temp = (inputSampleL * y.fixA[Y_FIX_A0]) + y.fixA[Y_FIX_S_L1];
		y.fixA[Y_FIX_S_L1] = (inputSampleL * y.fixA[Y_FIX_A1]) -
							 (temp * y.fixA[Y_FIX_B1]) + y.fixA[Y_FIX_S_L2];
		y.fixA[Y_FIX_S_L2] =
			(inputSampleL * y.fixA[Y_FIX_A2]) - (temp * y.fixA[Y_FIX_B2]);
		inputSampleL = temp; // fixed biquad filtering ultrasonics
		temp = (inputSampleR * y.fixA[Y_FIX_A0]) + y.fixA[Y_FIX_S_R1];
		y.fixA[Y_FIX_S_R1] = (inputSampleR * y.fixA[Y_FIX_A1]) -
							 (temp * y.fixA[Y_FIX_B1]) + y.fixA[Y_FIX_S_R2];
		y.fixA[Y_FIX_S_R2] =
			(inputSampleR * y.fixA[Y_FIX_A2]) - (temp * y.fixA[Y_FIX_B2]);
		inputSampleR = temp; // fixed biquad filtering ultrasonics

		// encode/decode courtesy of torridgristle under the MIT license
		if (inputSampleL > 1.0) inputSampleL = 1.0;
		else if (inputSampleL > 0.0)
			inputSampleL = 1.0 - pow(1.0 - inputSampleL, powFactor);
		if (inputSampleL < -1.0) inputSampleL = -1.0;
		else if (inputSampleL < 0.0)
			inputSampleL = -1.0 + pow(1.0 + inputSampleL, powFactor);
		if (inputSampleR > 1.0) inputSampleR = 1.0;
		else if (inputSampleR > 0.0)
			inputSampleR = 1.0 - pow(1.0 - inputSampleR, powFactor);
		if (inputSampleR < -1.0) inputSampleR = -1.0;
		else if (inputSampleR < 0.0)
			inputSampleR = -1.0 + pow(1.0 + inputSampleR, powFactor);

		temp = (inputSampleL * y.biquad[Y_BIQ_A0]) + y.biquad[Y_BIQ_S_L1];
		y.biquad[Y_BIQ_S_L1] = (inputSampleL * y.biquad[Y_BIQ_A1]) -
							   (temp * y.biquad[Y_BIQ_B1]) + y.biquad[Y_BIQ_S_L2];
		y.biquad[Y_BIQ_S_L2] =
			(inputSampleL * y.biquad[Y_BIQ_A2]) - (temp * y.biquad[Y_BIQ_B2]);
		inputSampleL = temp; // coefficient interpolating biquad filter
		temp = (inputSampleR * y.biquad[Y_BIQ_A0]) + y.biquad[Y_BIQ_S_R1];
		y.biquad[Y_BIQ_S_R1] = (inputSampleR * y.biquad[Y_BIQ_A1]) -
							   (temp * y.biquad[Y_BIQ_B1]) + y.biquad[Y_BIQ_S_R2];
		y.biquad[Y_BIQ_S_R2] =
			(inputSampleR * y.biquad[Y_BIQ_A2]) - (temp * y.biquad[Y_BIQ_B2]);
		inputSampleR = temp; // coefficient interpolating biquad filter

		// encode/decode courtesy of torridgristle under the MIT license
		if (inputSampleL > 1.0) inputSampleL = 1.0;
		else if (inputSampleL > 0.0)
			inputSampleL = 1.0 - pow(1.0 - inputSampleL, (1.0 / powFactor));
		if (inputSampleL < -1.0) inputSampleL = -1.0;
		else if (inputSampleL < 0.0)
			inputSampleL = -1.0 + pow(1.0 + inputSampleL, (1.0 / powFactor));
		if (inputSampleR > 1.0) inputSampleR = 1.0;
		else if (inputSampleR > 0.0)
			inputSampleR = 1.0 - pow(1.0 - inputSampleR, (1.0 / powFactor));
		if (inputSampleR < -1.0) inputSampleR = -1.0;
		else if (inputSampleR < 0.0)
			inputSampleR = -1.0 + pow(1.0 + inputSampleR, (1.0 / powFactor));

		inputSampleL *= outTrim;
		inputSampleR *= outTrim;

		temp = (inputSampleL * y.fixB[Y_FIX_A0]) + y.fixB[Y_FIX_S_L1];
		y.fixB[Y_FIX_S_L1] = (inputSampleL * y.fixB[Y_FIX_A1]) -
							 (temp * y.fixB[Y_FIX_B1]) + y.fixB[Y_FIX_S_L2];
		y.fixB[Y_FIX_S_L2] =
			(inputSampleL * y.fixB[Y_FIX_A2]) - (temp * y.fixB[Y_FIX_B2]);
		inputSampleL = temp; // fixed biquad filtering ultrasonics
		temp = (inputSampleR * y.fixB[Y_FIX_A0]) + y.fixB[Y_FIX_S_R1];
		y.fixB[Y_FIX_S_R1] = (inputSampleR * y.fixB[Y_FIX_A1]) -
							 (temp * y.fixB[Y_FIX_B1]) + y.fixB[Y_FIX_S_R2];
		y.fixB[Y_FIX_S_R2] =
			(inputSampleR * y.fixB[Y_FIX_A2]) - (temp * y.fixB[Y_FIX_B2]);
		inputSampleR = temp; // fixed biquad filtering ultrasonics

		if (wet < 1.0) {
			inputSampleL = (inputSampleL * wet) + (drySampleL * (1.0 - wet));
			inputSampleR = (inputSampleR * wet) + (drySampleR * (1.0 - wet));
		}

		// begin 32 bit stereo floating point dither
		int expon;
		frexpf((float)inputSampleL, &expon);
		y.fpdL ^= y.fpdL << 13;
		y.fpdL ^= y.fpdL >> 17;
		y.fpdL ^= y.fpdL << 5;
		inputSampleL +=
			((double(y.fpdL) - uint32_t(0x7fffffff)) * 5.5e-36l * pow(2, expon + 62));
		frexpf((float)inputSampleR, &expon);
		y.fpdR ^= y.fpdR << 13;
		y.fpdR ^= y.fpdR >> 17;
		y.fpdR ^= y.fpdR << 5;
		inputSampleR +=
			((double(y.fpdR) - uint32_t(0x7fffffff)) * 5.5e-36l * pow(2, expon + 62));
		// end 32 bit stereo floating point dither

		*out1 = inputSampleL;
		*out2 = inputSampleR;

		in1++;
		in2++;
		out1++;
		out2++;
	}
}

//////////////
// BANDPASS //
//////////////

struct ybandpass {
	double samplerate;
	array<double, Y_BIQ_TOTAL> biquad;

	double powFactorA;
	double powFactorB;
	double inTrimA;
	double inTrimB;
	double outTrimA;
	double outTrimB;

	array<double, Y_FIX_TOTAL> fixA;
	array<double, Y_FIX_TOTAL> fixB;

	uint32_t fpdL;
	uint32_t fpdR;
	// default stuff

	float drive;
	float frequency;
	float resonance;
	float edge;
	float output;
	float mix; // parameters. Always 0-1, and we scale/alter them elsewhere.
};

inline void ybandpass_init(ybandpass& y, double samplerate)
{
	y.samplerate = samplerate;
	y.drive = 0.1f;
	y.frequency = 1.0f;
	y.resonance = 0.02f;
	y.edge = 0.1f;
	y.output = 0.9f;
	y.mix = 1.0f;
	y.fpdL = 0;
	y.fpdR = 0;
	y.biquad.fill(0);

	for (int x = 0; x < Y_BIQ_TOTAL; x++) { y.biquad[x] = 0.0; }
	y.powFactorA = 1.0;
	y.powFactorB = 1.0;
	y.inTrimA = 0.1;
	y.inTrimB = 0.1;
	y.outTrimA = 1.0;
	y.outTrimB = 1.0;
	for (int x = 0; x < Y_FIX_TOTAL; x++) {
		y.fixA[x] = 0.0;
		y.fixB[x] = 0.0;
	}

	y.fpdL = 1.0;
	while (y.fpdL < 16386) y.fpdL = rand() * UINT32_MAX;
	y.fpdR = 1.0;
	while (y.fpdR < 16386) y.fpdR = rand() * UINT32_MAX;
}

template <typename t_sample>
inline void ybandpass_process_block(ybandpass& y, t_sample** inputs, t_sample** outputs,
									int blockSize)
{
	t_sample* in1 = inputs[0];
	t_sample* in2 = inputs[1];
	t_sample* out1 = outputs[0];
	t_sample* out2 = outputs[1];

	int inFramesToProcess = blockSize;
	double overallscale = 1.0;
	overallscale /= 44100.0;
	overallscale *= y.samplerate;

	y.inTrimA = y.inTrimB;
	y.inTrimB = y.drive * 10.0;

	y.biquad[Y_BIQ_FREQ] = pow(y.frequency, 3) * 20000.0;
	if (y.biquad[Y_BIQ_FREQ] < 15.0) y.biquad[Y_BIQ_FREQ] = 15.0;
	y.biquad[Y_BIQ_FREQ] /= y.samplerate;
	y.biquad[Y_BIQ_RESO] = (pow(y.resonance, 2) * 15.0) + 0.5571;
	y.biquad[Y_BIQ_A_A0] = y.biquad[Y_BIQ_A_B0];
	// y.biquad[biq_aA1] = y.biquad[biq_aB1];
	y.biquad[Y_BIQ_A_A2] = y.biquad[Y_BIQ_A_B2];
	y.biquad[Y_BIQ_B_A1] = y.biquad[Y_BIQ_B_B1];
	y.biquad[Y_BIQ_B_A2] = y.biquad[Y_BIQ_B_B2];
	// previous run through the buffer is still in the filter, so we move it
	// to the A section and now it's the new starting point.
	double K = tan(M_PI * y.biquad[Y_BIQ_FREQ]);
	double norm = 1.0 / (1.0 + K / y.biquad[Y_BIQ_RESO] + K * K);
	y.biquad[Y_BIQ_A_B0] = K / y.biquad[Y_BIQ_RESO] * norm;
	// y.biquad[y.biq_aB1] = 0.0; //bandpass can simplify the biquad kernel: leave out
	// this multiply
	y.biquad[Y_BIQ_A_B2] = -y.biquad[Y_BIQ_A_B0];
	y.biquad[Y_BIQ_B_B1] = 2.0 * (K * K - 1.0) * norm;
	y.biquad[Y_BIQ_B_B2] = (1.0 - K / y.biquad[Y_BIQ_RESO] + K * K) * norm;
	// for the coefficient-interpolated biquad filter

	y.powFactorA = y.powFactorB;
	y.powFactorB = pow(y.drive + 0.9, 4);

	// 1.0 == target neutral

	y.outTrimA = y.outTrimB;
	y.outTrimB = y.outTrimB;

	double wet = y.mix;

	y.fixA[Y_FIX_FREQ] = y.fixB[Y_FIX_FREQ] = 20000.0 / y.samplerate;
	y.fixA[Y_FIX_RESO] = y.fixB[Y_FIX_RESO] = 0.7071; // butterworth Q

	K = tan(M_PI * y.fixA[Y_FIX_FREQ]);
	norm = 1.0 / (1.0 + K / y.fixA[Y_FIX_RESO] + K * K);
	y.fixA[Y_FIX_A0] = y.fixB[Y_FIX_A0] = K * K * norm;
	y.fixA[Y_FIX_A1] = y.fixB[Y_FIX_A1] = 2.0 * y.fixA[Y_FIX_A0];
	y.fixA[Y_FIX_A2] = y.fixB[Y_FIX_A2] = y.fixA[Y_FIX_A0];
	y.fixA[Y_FIX_B1] = y.fixB[Y_FIX_B1] = 2.0 * (K * K - 1.0) * norm;
	y.fixA[Y_FIX_B2] = y.fixB[Y_FIX_B2] = (1.0 - K / y.fixA[Y_FIX_RESO] + K * K) * norm;
	// for the fixed-position biquad filter

	for (int s = 0; s < blockSize; s++) {
		double inputSampleL = *in1;
		double inputSampleR = *in2;
		if (fabs(inputSampleL) < 1.18e-23) inputSampleL = y.fpdL * 1.18e-17;
		if (fabs(inputSampleR) < 1.18e-23) inputSampleR = y.fpdR * 1.18e-17;
		double drySampleL = inputSampleL;
		double drySampleR = inputSampleR;

		double temp = (double)s / inFramesToProcess;
		y.biquad[Y_BIQ_A0] =
			(y.biquad[Y_BIQ_A_A0] * temp) + (y.biquad[Y_BIQ_A_B0] * (1.0 - temp));
		// biquad[biq_a1] = (biquad[biq_aA1]*temp)+(biquad[biq_aB1]*(1.0-temp));
		y.biquad[Y_BIQ_A2] =
			(y.biquad[Y_BIQ_A_A2] * temp) + (y.biquad[Y_BIQ_A_B2] * (1.0 - temp));
		y.biquad[Y_BIQ_B1] =
			(y.biquad[Y_BIQ_B_A1] * temp) + (y.biquad[Y_BIQ_B_B1] * (1.0 - temp));
		y.biquad[Y_BIQ_B2] =
			(y.biquad[Y_BIQ_B_A2] * temp) + (y.biquad[Y_BIQ_B_B2] * (1.0 - temp));
		// this is the interpolation code for the biquad
		double powFactor = (y.powFactorA * temp) + (y.powFactorB * (1.0 - temp));
		double inTrim = (y.inTrimA * temp) + (y.inTrimB * (1.0 - temp));
		double outTrim = (y.outTrimA * temp) + (y.outTrimB * (1.0 - temp));

		inputSampleL *= inTrim;
		inputSampleR *= inTrim;

		temp = (inputSampleL * y.fixA[Y_FIX_A0]) + y.fixA[Y_FIX_S_L1];
		y.fixA[Y_FIX_S_L1] = (inputSampleL * y.fixA[Y_FIX_A1]) -
							 (temp * y.fixA[Y_FIX_B1]) + y.fixA[Y_FIX_S_L2];
		y.fixA[Y_FIX_S_L2] =
			(inputSampleL * y.fixA[Y_FIX_A2]) - (temp * y.fixA[Y_FIX_B2]);
		inputSampleL = temp; // fixed biquad filtering ultrasonics
		temp = (inputSampleR * y.fixA[Y_FIX_A0]) + y.fixA[Y_FIX_S_R1];
		y.fixA[Y_FIX_S_R1] = (inputSampleR * y.fixA[Y_FIX_A1]) -
							 (temp * y.fixA[Y_FIX_B1]) + y.fixA[Y_FIX_S_R2];
		y.fixA[Y_FIX_S_R2] =
			(inputSampleR * y.fixA[Y_FIX_A2]) - (temp * y.fixA[Y_FIX_B2]);
		inputSampleR = temp; // fixed biquad filtering ultrasonics

		// encode/decode courtesy of torridgristle under the MIT license
		if (inputSampleL > 1.0) inputSampleL = 1.0;
		else if (inputSampleL > 0.0)
			inputSampleL = 1.0 - pow(1.0 - inputSampleL, powFactor);
		if (inputSampleL < -1.0) inputSampleL = -1.0;
		else if (inputSampleL < 0.0)
			inputSampleL = -1.0 + pow(1.0 + inputSampleL, powFactor);
		if (inputSampleR > 1.0) inputSampleR = 1.0;
		else if (inputSampleR > 0.0)
			inputSampleR = 1.0 - pow(1.0 - inputSampleR, powFactor);
		if (inputSampleR < -1.0) inputSampleR = -1.0;
		else if (inputSampleR < 0.0)
			inputSampleR = -1.0 + pow(1.0 + inputSampleR, powFactor);

		temp = (inputSampleL * y.biquad[Y_BIQ_A0]) + y.biquad[Y_BIQ_S_L1];
		y.biquad[Y_BIQ_S_L1] = -(temp * y.biquad[Y_BIQ_B1]) + y.biquad[Y_BIQ_S_L2];
		y.biquad[Y_BIQ_S_L2] =
			(inputSampleL * y.biquad[Y_BIQ_A2]) - (temp * y.biquad[Y_BIQ_B2]);
		inputSampleL = temp; // coefficient interpolating biquad filter
		temp = (inputSampleR * y.biquad[Y_BIQ_A0]) + y.biquad[Y_BIQ_S_R1];
		y.biquad[Y_BIQ_S_R1] = -(temp * y.biquad[Y_BIQ_B1]) + y.biquad[Y_BIQ_S_R2];
		y.biquad[Y_BIQ_S_R2] =
			(inputSampleR * y.biquad[Y_BIQ_A2]) - (temp * y.biquad[Y_BIQ_B2]);
		inputSampleR = temp; // coefficient interpolating biquad filter

		// encode/decode courtesy of torridgristle under the MIT license
		if (inputSampleL > 1.0) inputSampleL = 1.0;
		else if (inputSampleL > 0.0)
			inputSampleL = 1.0 - pow(1.0 - inputSampleL, (1.0 / powFactor));
		if (inputSampleL < -1.0) inputSampleL = -1.0;
		else if (inputSampleL < 0.0)
			inputSampleL = -1.0 + pow(1.0 + inputSampleL, (1.0 / powFactor));
		if (inputSampleR > 1.0) inputSampleR = 1.0;
		else if (inputSampleR > 0.0)
			inputSampleR = 1.0 - pow(1.0 - inputSampleR, (1.0 / powFactor));
		if (inputSampleR < -1.0) inputSampleR = -1.0;
		else if (inputSampleR < 0.0)
			inputSampleR = -1.0 + pow(1.0 + inputSampleR, (1.0 / powFactor));

		inputSampleL *= outTrim;
		inputSampleR *= outTrim;

		temp = (inputSampleL * y.fixB[Y_FIX_A0]) + y.fixB[Y_FIX_S_L1];
		y.fixB[Y_FIX_S_L1] = (inputSampleL * y.fixB[Y_FIX_A1]) -
							 (temp * y.fixB[Y_FIX_B1]) + y.fixB[Y_FIX_S_L2];
		y.fixB[Y_FIX_S_L2] =
			(inputSampleL * y.fixB[Y_FIX_A2]) - (temp * y.fixB[Y_FIX_B2]);
		inputSampleL = temp; // fixed biquad filtering ultrasonics
		temp = (inputSampleR * y.fixB[Y_FIX_A0]) + y.fixB[Y_FIX_S_R1];
		y.fixB[Y_FIX_S_R1] = (inputSampleR * y.fixB[Y_FIX_A1]) -
							 (temp * y.fixB[Y_FIX_B1]) + y.fixB[Y_FIX_S_R2];
		y.fixB[Y_FIX_S_R2] =
			(inputSampleR * y.fixB[Y_FIX_A2]) - (temp * y.fixB[Y_FIX_B2]);
		inputSampleR = temp; // fixed biquad filtering ultrasonics

		if (wet < 1.0) {
			inputSampleL = (inputSampleL * wet) + (drySampleL * (1.0 - wet));
			inputSampleR = (inputSampleR * wet) + (drySampleR * (1.0 - wet));
		}

		// begin 32 bit stereo floating point dither
		int expon;
		frexpf((float)inputSampleL, &expon);
		y.fpdL ^= y.fpdL << 13;
		y.fpdL ^= y.fpdL >> 17;
		y.fpdL ^= y.fpdL << 5;
		inputSampleL +=
			((double(y.fpdL) - uint32_t(0x7fffffff)) * 5.5e-36l * pow(2, expon + 62));
		frexpf((float)inputSampleR, &expon);
		y.fpdR ^= y.fpdR << 13;
		y.fpdR ^= y.fpdR >> 17;
		y.fpdR ^= y.fpdR << 5;
		inputSampleR +=
			((double(y.fpdR) - uint32_t(0x7fffffff)) * 5.5e-36l * pow(2, expon + 62));
		// end 32 bit stereo floating point dither

		*out1 = inputSampleL;
		*out2 = inputSampleR;

		in1++;
		in2++;
		out1++;
		out2++;
	}
}

///////////
// NOTCH //
///////////

struct ynotch {
	double samplerate;
	array<double, Y_BIQ_TOTAL> biquad;

	double powFactorA;
	double powFactorB;
	double inTrimA;
	double inTrimB;
	double outTrimA;
	double outTrimB;

	array<double, Y_FIX_TOTAL> fixA;
	array<double, Y_FIX_TOTAL> fixB;

	uint32_t fpdL;
	uint32_t fpdR;
	// default stuff

	float drive;
	float frequency;
	float resonance;
	float edge;
	float output;
	float mix; // parameters. Always 0-1, and we scale/alter them elsewhere.
};

inline void ynotch_init(ynotch& y, double samplerate)
{
	y.samplerate = samplerate;
	y.drive = 0.1f;
	y.frequency = 1.0f;
	y.resonance = 0.0f;
	y.edge = 0.1f;
	y.output = 0.9f;
	y.mix = 1.0f;
	y.fpdL = 0;
	y.fpdR = 0;
	y.biquad.fill(0);

	y.powFactorA = 1.0;
	y.powFactorB = 1.0;
	y.inTrimA = 0.1;
	y.inTrimB = 0.1;
	y.outTrimA = 1.0;
	y.outTrimB = 1.0;
	for (int x = 0; x < Y_FIX_TOTAL; x++) {
		y.fixA[x] = 0.0;
		y.fixB[x] = 0.0;
	}

	y.fpdL = 1.0;
	while (y.fpdL < 16386) y.fpdL = rand() * UINT32_MAX;
	y.fpdR = 1.0;
	while (y.fpdR < 16386) y.fpdR = rand() * UINT32_MAX;
}

template <typename t_sample>
inline void ynotch_process_block(ynotch& y, t_sample** inputs, t_sample** outputs,
								 int blockSize)
{
	t_sample* in1 = inputs[0];
	t_sample* in2 = inputs[1];
	t_sample* out1 = outputs[0];
	t_sample* out2 = outputs[1];

	int inFramesToProcess = blockSize;
	double overallscale = 1.0;
	overallscale /= 44100.0;
	overallscale *= y.samplerate;

	y.inTrimA = y.inTrimB;
	y.inTrimB = y.drive * 10.0;

	y.biquad[Y_BIQ_FREQ] = pow(y.frequency, 3) * 20000.0;
	if (y.biquad[Y_BIQ_FREQ] < 15.0) y.biquad[Y_BIQ_FREQ] = 15.0;
	y.biquad[Y_BIQ_FREQ] /= y.samplerate;
	y.biquad[Y_BIQ_RESO] = (pow(y.resonance, 2) * 15.0) + 0.0001;
	y.biquad[Y_BIQ_A_A0] = y.biquad[Y_BIQ_A_B0];
	y.biquad[Y_BIQ_A_A1] = y.biquad[Y_BIQ_A_B1];
	y.biquad[Y_BIQ_A_A2] = y.biquad[Y_BIQ_A_B2];
	y.biquad[Y_BIQ_B_A1] = y.biquad[Y_BIQ_B_B1];
	y.biquad[Y_BIQ_B_A2] = y.biquad[Y_BIQ_B_B2];
	// previous run through the buffer is still in the filter, so we move it
	// to the A section and now it's the new starting point.
	double K = tan(M_PI * y.biquad[Y_BIQ_FREQ]);
	double norm = 1.0 / (1.0 + K / y.biquad[Y_BIQ_RESO] + K * K);
	y.biquad[Y_BIQ_A_B0] = (1.0 + K * K) * norm;
	y.biquad[Y_BIQ_A_B1] = 2.0 * (K * K - 1) * norm;
	y.biquad[Y_BIQ_A_B2] = y.biquad[Y_BIQ_A_B0];
	y.biquad[Y_BIQ_B_B1] = y.biquad[Y_BIQ_A_B1];
	y.biquad[Y_BIQ_B_B2] = (1.0 - K / y.biquad[Y_BIQ_RESO] + K * K) * norm;
	// for the coefficient-interpolated biquad filter

	y.powFactorA = y.powFactorB;
	y.powFactorB = pow(y.edge + 0.9, 4);

	// 1.0 == target neutral

	y.outTrimA = y.outTrimB;
	y.outTrimB = y.output;

	double wet = y.mix;

	y.fixA[Y_FIX_FREQ] = y.fixB[Y_FIX_FREQ] = 20000.0 / y.samplerate;
	y.fixA[Y_FIX_RESO] = y.fixB[Y_FIX_RESO] = 0.7071; // butterworth Q

	K = tan(M_PI * y.fixA[Y_FIX_FREQ]);
	norm = 1.0 / (1.0 + K / y.fixA[Y_FIX_RESO] + K * K);
	y.fixA[Y_FIX_A0] = y.fixB[Y_FIX_A0] = K * K * norm;
	y.fixA[Y_FIX_A1] = y.fixB[Y_FIX_A1] = 2.0 * y.fixA[Y_FIX_A0];
	y.fixA[Y_FIX_A2] = y.fixB[Y_FIX_A2] = y.fixA[Y_FIX_A0];
	y.fixA[Y_FIX_B1] = y.fixB[Y_FIX_B1] = 2.0 * (K * K - 1.0) * norm;
	y.fixA[Y_FIX_B2] = y.fixB[Y_FIX_B2] = (1.0 - K / y.fixA[Y_FIX_RESO] + K * K) * norm;
	// for the fixed-position biquad filter

	for (int s = 0; s < blockSize; s++) {
		double inputSampleL = *in1;
		double inputSampleR = *in2;
		if (fabs(inputSampleL) < 1.18e-23) inputSampleL = y.fpdL * 1.18e-17;
		if (fabs(inputSampleR) < 1.18e-23) inputSampleR = y.fpdR * 1.18e-17;
		double drySampleL = inputSampleL;
		double drySampleR = inputSampleR;

		double temp = (double)s / inFramesToProcess;
		y.biquad[Y_BIQ_A0] =
			(y.biquad[Y_BIQ_A_A0] * temp) + (y.biquad[Y_BIQ_A_B0] * (1.0 - temp));
		y.biquad[Y_BIQ_A1] =
			(y.biquad[Y_BIQ_A_A1] * temp) + (y.biquad[Y_BIQ_A_B1] * (1.0 - temp));
		y.biquad[Y_BIQ_A2] =
			(y.biquad[Y_BIQ_A_A2] * temp) + (y.biquad[Y_BIQ_A_B2] * (1.0 - temp));
		y.biquad[Y_BIQ_B1] =
			(y.biquad[Y_BIQ_B_A1] * temp) + (y.biquad[Y_BIQ_B_B1] * (1.0 - temp));
		y.biquad[Y_BIQ_B2] =
			(y.biquad[Y_BIQ_B_A2] * temp) + (y.biquad[Y_BIQ_B_B2] * (1.0 - temp));
		// this is the interpolation code for the biquad
		double powFactor = (y.powFactorA * temp) + (y.powFactorB * (1.0 - temp));
		double inTrim = (y.inTrimA * temp) + (y.inTrimB * (1.0 - temp));
		double outTrim = (y.outTrimA * temp) + (y.outTrimB * (1.0 - temp));

		inputSampleL *= inTrim;
		inputSampleR *= inTrim;

		temp = (inputSampleL * y.fixA[Y_FIX_A0]) + y.fixA[Y_FIX_S_L1];
		y.fixA[Y_FIX_S_L1] = (inputSampleL * y.fixA[Y_FIX_A1]) -
							 (temp * y.fixA[Y_FIX_B1]) + y.fixA[Y_FIX_S_L2];
		y.fixA[Y_FIX_S_L2] =
			(inputSampleL * y.fixA[Y_FIX_A2]) - (temp * y.fixA[Y_FIX_B2]);
		inputSampleL = temp; // fixed biquad filtering ultrasonics
		temp = (inputSampleR * y.fixA[Y_FIX_A0]) + y.fixA[Y_FIX_S_R1];
		y.fixA[Y_FIX_S_R1] = (inputSampleR * y.fixA[Y_FIX_A1]) -
							 (temp * y.fixA[Y_FIX_B1]) + y.fixA[Y_FIX_S_R2];
		y.fixA[Y_FIX_S_R2] =
			(inputSampleR * y.fixA[Y_FIX_A2]) - (temp * y.fixA[Y_FIX_B2]);
		inputSampleR = temp; // fixed biquad filtering ultrasonics

		// encode/decode courtesy of torridgristle under the MIT license
		if (inputSampleL > 1.0) inputSampleL = 1.0;
		else if (inputSampleL > 0.0)
			inputSampleL = 1.0 - pow(1.0 - inputSampleL, powFactor);
		if (inputSampleL < -1.0) inputSampleL = -1.0;
		else if (inputSampleL < 0.0)
			inputSampleL = -1.0 + pow(1.0 + inputSampleL, powFactor);
		if (inputSampleR > 1.0) inputSampleR = 1.0;
		else if (inputSampleR > 0.0)
			inputSampleR = 1.0 - pow(1.0 - inputSampleR, powFactor);
		if (inputSampleR < -1.0) inputSampleR = -1.0;
		else if (inputSampleR < 0.0)
			inputSampleR = -1.0 + pow(1.0 + inputSampleR, powFactor);

		temp = (inputSampleL * y.biquad[Y_BIQ_A0]) + y.biquad[Y_BIQ_S_L1];
		y.biquad[Y_BIQ_S_L1] = (inputSampleL * y.biquad[Y_BIQ_A1]) -
							   (temp * y.biquad[Y_BIQ_B1]) + y.biquad[Y_BIQ_S_L2];
		y.biquad[Y_BIQ_S_L2] =
			(inputSampleL * y.biquad[Y_BIQ_A2]) - (temp * y.biquad[Y_BIQ_B2]);
		inputSampleL = temp; // coefficient interpolating biquad filter
		temp = (inputSampleR * y.biquad[Y_BIQ_A0]) + y.biquad[Y_BIQ_S_R1];
		y.biquad[Y_BIQ_S_R1] = (inputSampleR * y.biquad[Y_BIQ_A1]) -
							   (temp * y.biquad[Y_BIQ_B1]) + y.biquad[Y_BIQ_S_R2];
		y.biquad[Y_BIQ_S_R2] =
			(inputSampleR * y.biquad[Y_BIQ_A2]) - (temp * y.biquad[Y_BIQ_B2]);
		inputSampleR = temp; // coefficient interpolating biquad filter

		// encode/decode courtesy of torridgristle under the MIT license
		if (inputSampleL > 1.0) inputSampleL = 1.0;
		else if (inputSampleL > 0.0)
			inputSampleL = 1.0 - pow(1.0 - inputSampleL, (1.0 / powFactor));
		if (inputSampleL < -1.0) inputSampleL = -1.0;
		else if (inputSampleL < 0.0)
			inputSampleL = -1.0 + pow(1.0 + inputSampleL, (1.0 / powFactor));
		if (inputSampleR > 1.0) inputSampleR = 1.0;
		else if (inputSampleR > 0.0)
			inputSampleR = 1.0 - pow(1.0 - inputSampleR, (1.0 / powFactor));
		if (inputSampleR < -1.0) inputSampleR = -1.0;
		else if (inputSampleR < 0.0)
			inputSampleR = -1.0 + pow(1.0 + inputSampleR, (1.0 / powFactor));

		inputSampleL *= outTrim;
		inputSampleR *= outTrim;

		temp = (inputSampleL * y.fixB[Y_FIX_A0]) + y.fixB[Y_FIX_S_L1];
		y.fixB[Y_FIX_S_L1] = (inputSampleL * y.fixB[Y_FIX_A1]) -
							 (temp * y.fixB[Y_FIX_B1]) + y.fixB[Y_FIX_S_L2];
		y.fixB[Y_FIX_S_L2] =
			(inputSampleL * y.fixB[Y_FIX_A2]) - (temp * y.fixB[Y_FIX_B2]);
		inputSampleL = temp; // fixed biquad filtering ultrasonics
		temp = (inputSampleR * y.fixB[Y_FIX_A0]) + y.fixB[Y_FIX_S_R1];
		y.fixB[Y_FIX_S_R1] = (inputSampleR * y.fixB[Y_FIX_A1]) -
							 (temp * y.fixB[Y_FIX_B1]) + y.fixB[Y_FIX_S_R2];
		y.fixB[Y_FIX_S_R2] =
			(inputSampleR * y.fixB[Y_FIX_A2]) - (temp * y.fixB[Y_FIX_B2]);
		inputSampleR = temp; // fixed biquad filtering ultrasonics

		if (wet < 1.0) {
			inputSampleL = (inputSampleL * wet) + (drySampleL * (1.0 - wet));
			inputSampleR = (inputSampleR * wet) + (drySampleR * (1.0 - wet));
		}

		// begin 32 bit stereo floating point dither
		int expon;
		frexpf((float)inputSampleL, &expon);
		y.fpdL ^= y.fpdL << 13;
		y.fpdL ^= y.fpdL >> 17;
		y.fpdL ^= y.fpdL << 5;
		inputSampleL +=
			((double(y.fpdL) - uint32_t(0x7fffffff)) * 5.5e-36l * pow(2, expon + 62));
		frexpf((float)inputSampleR, &expon);
		y.fpdR ^= y.fpdR << 13;
		y.fpdR ^= y.fpdR >> 17;
		y.fpdR ^= y.fpdR << 5;
		inputSampleR +=
			((double(y.fpdR) - uint32_t(0x7fffffff)) * 5.5e-36l * pow(2, expon + 62));
		// end 32 bit stereo floating point dither

		*out1 = inputSampleL;
		*out2 = inputSampleR;

		in1++;
		in2++;
		out1++;
		out2++;
	}
}

/////////
// SVF //
/////////

enum ysvf_types {
	Y_LOWPASS,
	Y_HIGHPASS,
	Y_BANDPASS,
	Y_NOTCH
};

struct ysvf {
	ysvf_types filter_type;
	ylowpass lowpass;
	yhighpass highpass;
	ybandpass bandpass;
	ynotch notch;
};

enum ysvf_parameters {
	Y_DRIVE,
	Y_FREQUENCY,
	Y_RESONANCE,
	Y_EDGE,
};

inline void ysvf_init(ysvf& y, double samplerate = 44100)
{
	ylowpass_init(y.lowpass, samplerate);
	yhighpass_init(y.highpass, samplerate);
	ybandpass_init(y.bandpass, samplerate);
	ynotch_init(y.notch, samplerate);
	y.filter_type = ysvf_types::Y_LOWPASS;
}

inline void ysvf_set_param(ysvf& y, ysvf_parameters param, float value)
{
	switch (param) {
	case Y_DRIVE:
		y.lowpass.drive = value;
		y.highpass.drive = value;
		y.bandpass.drive = value;
		y.notch.drive = value;
		break;
	case Y_FREQUENCY:
		y.lowpass.frequency = value;
		y.highpass.frequency = value;
		y.bandpass.frequency = value;
		y.notch.frequency = value;
		break;
	case Y_RESONANCE:
		y.lowpass.resonance = value;
		y.highpass.resonance = value;
		y.bandpass.resonance = value;
		y.notch.resonance = value;
		break;
	case Y_EDGE:
		y.lowpass.edge = value;
		y.highpass.edge = value;
		y.bandpass.edge = value;
		y.notch.edge = value;
		break;
	}
}

template <typename t_sample>
inline void y_process_samples(ysvf& y, t_sample** inputs, t_sample** outputs,
							  int block_size)
{
	switch (y.filter_type) {
	case Y_LOWPASS:
		ylowpass_process_block(y.lowpass, inputs, outputs, block_size);
		break;
	case ysvf_types::Y_HIGHPASS:
		yhighpass_process_block(y.highpass, inputs, outputs, block_size);
		break;
	case ysvf_types::Y_BANDPASS:
		ybandpass_process_block(y.bandpass, inputs, outputs, block_size);
		break;
	case ysvf_types::Y_NOTCH:
		ynotch_process_block(y.notch, inputs, outputs, block_size);
		break;
	}
}
} // namespace trnr

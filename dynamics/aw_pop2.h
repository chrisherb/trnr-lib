#pragma once
#include <cstdlib>
#include <stdint.h>

namespace trnr {
// compressor based on pop2 by Chris Johnson
class aw_pop2 {
public:
	aw_pop2()
	{
		samplerate = 44100;

		A = 0.5;
		B = 0.5;
		C = 0.5;
		D = 0.5;
		E = 1.0;
		fpdL = 1.0;
		while (fpdL < 16386) fpdL = rand() * UINT32_MAX;
		fpdR = 1.0;
		while (fpdR < 16386) fpdR = rand() * UINT32_MAX;

		lastSampleL = 0.0;
		wasPosClipL = false;
		wasNegClipL = false;
		lastSampleR = 0.0;
		wasPosClipR = false;
		wasNegClipR = false;
		for (int x = 0; x < 16; x++) {
			intermediateL[x] = 0.0;
			intermediateR[x] = 0.0;
		}

		muVaryL = 0.0;
		muAttackL = 0.0;
		muNewSpeedL = 1000.0;
		muSpeedAL = 1000.0;
		muSpeedBL = 1000.0;
		muCoefficientAL = 1.0;
		muCoefficientBL = 1.0;

		muVaryR = 0.0;
		muAttackR = 0.0;
		muNewSpeedR = 1000.0;
		muSpeedAR = 1000.0;
		muSpeedBR = 1000.0;
		muCoefficientAR = 1.0;
		muCoefficientBR = 1.0;

		flip = false;
		// this is reset: values being initialized only once. Startup values, whatever they are.
	}

	void set_compression(double value) { A = clamp(value); }

	void set_attack(double value) { B = clamp(value); }

	void set_release(double value) { C = clamp(value); }

	void set_drive(double value) { D = clamp(value); }

	void set_drywet(double value) { E = clamp(value); }

	void set_samplerate(double _samplerate) { samplerate = _samplerate; }

	template <typename t_sample>
	void process_block(t_sample** inputs, t_sample** outputs, long sampleframes)
	{
		t_sample* in1 = inputs[0];
		t_sample* in2 = inputs[1];
		t_sample* out1 = outputs[0];
		t_sample* out2 = outputs[1];

		double overallscale = 1.0;
		overallscale /= 44100.0;
		overallscale *= samplerate;

		int spacing = floor(overallscale); // should give us working basic scaling, usually 2 or 4
		if (spacing < 1) spacing = 1;
		if (spacing > 16) spacing = 16;

		double threshold = 1.0 - ((1.0 - pow(1.0 - A, 2)) * 0.9);
		double attack = ((pow(B, 4) * 100000.0) + 10.0) * overallscale;
		double release = ((pow(C, 5) * 2000000.0) + 20.0) * overallscale;
		double maxRelease = release * 4.0;
		double muPreGain = 1.0 / threshold;
		double muMakeupGain = sqrt(1.0 / threshold) * D;
		double wet = E;
		// compressor section

		while (--sampleframes >= 0) {
			double inputSampleL = *in1;
			double inputSampleR = *in2;
			if (fabs(inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
			if (fabs(inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
			double drySampleL = inputSampleL;
			double drySampleR = inputSampleR;

			// begin compressor section
			inputSampleL *= muPreGain;
			inputSampleR *= muPreGain;
			// adjust coefficients for L
			if (flip) {
				if (fabs(inputSampleL) > threshold) {
					muVaryL = threshold / fabs(inputSampleL);
					muAttackL = sqrt(fabs(muSpeedAL));
					muCoefficientAL = muCoefficientAL * (muAttackL - 1.0);
					if (muVaryL < threshold) muCoefficientAL = muCoefficientAL + threshold;
					else muCoefficientAL = muCoefficientAL + muVaryL;
					muCoefficientAL = muCoefficientAL / muAttackL;
					muNewSpeedL = muSpeedAL * (muSpeedAL - 1.0);
					muNewSpeedL = muNewSpeedL + release;
					muSpeedAL = muNewSpeedL / muSpeedAL;
					if (muSpeedAL > maxRelease) muSpeedAL = maxRelease;
				} else {
					muCoefficientAL = muCoefficientAL * ((muSpeedAL * muSpeedAL) - 1.0);
					muCoefficientAL = muCoefficientAL + 1.0;
					muCoefficientAL = muCoefficientAL / (muSpeedAL * muSpeedAL);
					muNewSpeedL = muSpeedAL * (muSpeedAL - 1.0);
					muNewSpeedL = muNewSpeedL + attack;
					muSpeedAL = muNewSpeedL / muSpeedAL;
				}
			} else {
				if (fabs(inputSampleL) > threshold) {
					muVaryL = threshold / fabs(inputSampleL);
					muAttackL = sqrt(fabs(muSpeedBL));
					muCoefficientBL = muCoefficientBL * (muAttackL - 1);
					if (muVaryL < threshold) muCoefficientBL = muCoefficientBL + threshold;
					else muCoefficientBL = muCoefficientBL + muVaryL;
					muCoefficientBL = muCoefficientBL / muAttackL;
					muNewSpeedL = muSpeedBL * (muSpeedBL - 1.0);
					muNewSpeedL = muNewSpeedL + release;
					muSpeedBL = muNewSpeedL / muSpeedBL;
					if (muSpeedBL > maxRelease) muSpeedBL = maxRelease;
				} else {
					muCoefficientBL = muCoefficientBL * ((muSpeedBL * muSpeedBL) - 1.0);
					muCoefficientBL = muCoefficientBL + 1.0;
					muCoefficientBL = muCoefficientBL / (muSpeedBL * muSpeedBL);
					muNewSpeedL = muSpeedBL * (muSpeedBL - 1.0);
					muNewSpeedL = muNewSpeedL + attack;
					muSpeedBL = muNewSpeedL / muSpeedBL;
				}
			}
			// got coefficients, adjusted speeds for L

			// adjust coefficients for R
			if (flip) {
				if (fabs(inputSampleR) > threshold) {
					muVaryR = threshold / fabs(inputSampleR);
					muAttackR = sqrt(fabs(muSpeedAR));
					muCoefficientAR = muCoefficientAR * (muAttackR - 1.0);
					if (muVaryR < threshold) muCoefficientAR = muCoefficientAR + threshold;
					else muCoefficientAR = muCoefficientAR + muVaryR;
					muCoefficientAR = muCoefficientAR / muAttackR;
					muNewSpeedR = muSpeedAR * (muSpeedAR - 1.0);
					muNewSpeedR = muNewSpeedR + release;
					muSpeedAR = muNewSpeedR / muSpeedAR;
					if (muSpeedAR > maxRelease) muSpeedAR = maxRelease;
				} else {
					muCoefficientAR = muCoefficientAR * ((muSpeedAR * muSpeedAR) - 1.0);
					muCoefficientAR = muCoefficientAR + 1.0;
					muCoefficientAR = muCoefficientAR / (muSpeedAR * muSpeedAR);
					muNewSpeedR = muSpeedAR * (muSpeedAR - 1.0);
					muNewSpeedR = muNewSpeedR + attack;
					muSpeedAR = muNewSpeedR / muSpeedAR;
				}
			} else {
				if (fabs(inputSampleR) > threshold) {
					muVaryR = threshold / fabs(inputSampleR);
					muAttackR = sqrt(fabs(muSpeedBR));
					muCoefficientBR = muCoefficientBR * (muAttackR - 1);
					if (muVaryR < threshold) muCoefficientBR = muCoefficientBR + threshold;
					else muCoefficientBR = muCoefficientBR + muVaryR;
					muCoefficientBR = muCoefficientBR / muAttackR;
					muNewSpeedR = muSpeedBR * (muSpeedBR - 1.0);
					muNewSpeedR = muNewSpeedR + release;
					muSpeedBR = muNewSpeedR / muSpeedBR;
					if (muSpeedBR > maxRelease) muSpeedBR = maxRelease;
				} else {
					muCoefficientBR = muCoefficientBR * ((muSpeedBR * muSpeedBR) - 1.0);
					muCoefficientBR = muCoefficientBR + 1.0;
					muCoefficientBR = muCoefficientBR / (muSpeedBR * muSpeedBR);
					muNewSpeedR = muSpeedBR * (muSpeedBR - 1.0);
					muNewSpeedR = muNewSpeedR + attack;
					muSpeedBR = muNewSpeedR / muSpeedBR;
				}
			}
			// got coefficients, adjusted speeds for R

			if (flip) {
				inputSampleL *= pow(muCoefficientAL, 2);
				inputSampleR *= pow(muCoefficientAR, 2);
			} else {
				inputSampleL *= pow(muCoefficientBL, 2);
				inputSampleR *= pow(muCoefficientBR, 2);
			}
			inputSampleL *= muMakeupGain;
			inputSampleR *= muMakeupGain;
			flip = !flip;
			// end compressor section

			if (wet < 1.0) {
				inputSampleL = (drySampleL * (1.0 - wet)) + (inputSampleL * wet);
				inputSampleR = (drySampleR * (1.0 - wet)) + (inputSampleR * wet);
			}

			// begin 64 bit stereo floating point dither
			// int expon; frexp((double)inputSampleL, &expon);
			fpdL ^= fpdL << 13;
			fpdL ^= fpdL >> 17;
			fpdL ^= fpdL << 5;
			// inputSampleL += ((double(fpdL)-uint32_t(0x7fffffff)) * 1.1e-44l * pow(2,expon+62));
			// frexp((double)inputSampleR, &expon);
			fpdR ^= fpdR << 13;
			fpdR ^= fpdR >> 17;
			fpdR ^= fpdR << 5;
			// inputSampleR += ((double(fpdR)-uint32_t(0x7fffffff)) * 1.1e-44l * pow(2,expon+62));
			// end 64 bit stereo floating point dither

			*out1 = inputSampleL;
			*out2 = inputSampleR;

			in1++;
			in2++;
			out1++;
			out2++;
		}
	}

private:
	double samplerate;

	uint32_t fpdL;
	uint32_t fpdR;
	// default stuff

	double muVaryL;
	double muAttackL;
	double muNewSpeedL;
	double muSpeedAL;
	double muSpeedBL;
	double muCoefficientAL;
	double muCoefficientBL;

	double muVaryR;
	double muAttackR;
	double muNewSpeedR;
	double muSpeedAR;
	double muSpeedBR;
	double muCoefficientAR;
	double muCoefficientBR;

	bool flip;

	double lastSampleL;
	double intermediateL[16];
	bool wasPosClipL;
	bool wasNegClipL;
	double lastSampleR;
	double intermediateR[16];
	bool wasPosClipR;
	bool wasNegClipR; // Stereo ClipOnly2

	float A;
	float B;
	float C;
	float D;
	float E; // parameters. Always 0-1, and we scale/alter them elsewhere.

	double clamp(double& value)
	{
		if (value > 1) {
			value = 1;
		} else if (value < 0) {
			value = 0;
		}
		return value;
	}
};
} // namespace trnr
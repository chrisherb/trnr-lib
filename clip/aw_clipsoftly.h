#pragma once
#include <cstdlib>
#include <stdint.h>
#include <cmath>

namespace trnr {
// soft clipper based on ClipSoftly by Chris Johnson
class aw_clipsoftly {
public:
	aw_clipsoftly()
	{
		samplerate = 44100;

		lastSampleL = 0.0;
		lastSampleR = 0.0;
		for (int x = 0; x < 16; x++) {
			intermediateL[x] = 0.0;
			intermediateR[x] = 0.0;
		}
		fpdL = 1.0;
		while (fpdL < 16386) fpdL = rand() * UINT32_MAX;
		fpdR = 1.0;
		while (fpdR < 16386) fpdR = rand() * UINT32_MAX;
		// this is reset: values being initialized only once. Startup values, whatever they are.
	}

	void set_samplerate(double _samplerate) { samplerate = _samplerate; }

	template <typename t_sample>
	void process_block(t_sample** inputs, t_sample** outputs, long sample_frames)
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

		while (--sample_frames >= 0) {
			double inputSampleL = *in1;
			double inputSampleR = *in2;
			if (fabs(inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
			if (fabs(inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

			double softSpeed = fabs(inputSampleL);
			if (softSpeed < 1.0) softSpeed = 1.0;
			else softSpeed = 1.0 / softSpeed;
			if (inputSampleL > 1.57079633) inputSampleL = 1.57079633;
			if (inputSampleL < -1.57079633) inputSampleL = -1.57079633;
			inputSampleL = sin(inputSampleL) * 0.9549925859; // scale to what cliponly uses
			inputSampleL = (inputSampleL * softSpeed) + (lastSampleL * (1.0 - softSpeed));

			softSpeed = fabs(inputSampleR);
			if (softSpeed < 1.0) softSpeed = 1.0;
			else softSpeed = 1.0 / softSpeed;
			if (inputSampleR > 1.57079633) inputSampleR = 1.57079633;
			if (inputSampleR < -1.57079633) inputSampleR = -1.57079633;
			inputSampleR = sin(inputSampleR) * 0.9549925859; // scale to what cliponly uses
			inputSampleR = (inputSampleR * softSpeed) + (lastSampleR * (1.0 - softSpeed));

			intermediateL[spacing] = inputSampleL;
			inputSampleL = lastSampleL; // Latency is however many samples equals one 44.1k sample
			for (int x = spacing; x > 0; x--) intermediateL[x - 1] = intermediateL[x];
			lastSampleL = intermediateL[0]; // run a little buffer to handle this

			intermediateR[spacing] = inputSampleR;
			inputSampleR = lastSampleR; // Latency is however many samples equals one 44.1k sample
			for (int x = spacing; x > 0; x--) intermediateR[x - 1] = intermediateR[x];
			lastSampleR = intermediateR[0]; // run a little buffer to handle this

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

	double lastSampleL;
	double intermediateL[16];
	double lastSampleR;
	double intermediateR[16];
	uint32_t fpdL;
	uint32_t fpdR;
	// default stuff
};
} // namespace trnr
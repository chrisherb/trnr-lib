#pragma once
#include <cmath>
#include <cstdlib>

namespace trnr {
// Clipper based on ClipOnly2 by Chris Johnson
class aw_cliponly2 {
public:
	aw_cliponly2()
	{
		samplerate = 44100;

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

			// begin ClipOnly2 stereo as a little, compressed chunk that can be dropped into code
			if (inputSampleL > 4.0) inputSampleL = 4.0;
			if (inputSampleL < -4.0) inputSampleL = -4.0;
			if (wasPosClipL == true) { // current will be over
				if (inputSampleL < lastSampleL) lastSampleL = 0.7058208 + (inputSampleL * 0.2609148);
				else lastSampleL = 0.2491717 + (lastSampleL * 0.7390851);
			}
			wasPosClipL = false;
			if (inputSampleL > 0.9549925859) {
				wasPosClipL = true;
				inputSampleL = 0.7058208 + (lastSampleL * 0.2609148);
			}
			if (wasNegClipL == true) { // current will be -over
				if (inputSampleL > lastSampleL) lastSampleL = -0.7058208 + (inputSampleL * 0.2609148);
				else lastSampleL = -0.2491717 + (lastSampleL * 0.7390851);
			}
			wasNegClipL = false;
			if (inputSampleL < -0.9549925859) {
				wasNegClipL = true;
				inputSampleL = -0.7058208 + (lastSampleL * 0.2609148);
			}
			intermediateL[spacing] = inputSampleL;
			inputSampleL = lastSampleL; // Latency is however many samples equals one 44.1k sample
			for (int x = spacing; x > 0; x--) intermediateL[x - 1] = intermediateL[x];
			lastSampleL = intermediateL[0]; // run a little buffer to handle this

			if (inputSampleR > 4.0) inputSampleR = 4.0;
			if (inputSampleR < -4.0) inputSampleR = -4.0;
			if (wasPosClipR == true) { // current will be over
				if (inputSampleR < lastSampleR) lastSampleR = 0.7058208 + (inputSampleR * 0.2609148);
				else lastSampleR = 0.2491717 + (lastSampleR * 0.7390851);
			}
			wasPosClipR = false;
			if (inputSampleR > 0.9549925859) {
				wasPosClipR = true;
				inputSampleR = 0.7058208 + (lastSampleR * 0.2609148);
			}
			if (wasNegClipR == true) { // current will be -over
				if (inputSampleR > lastSampleR) lastSampleR = -0.7058208 + (inputSampleR * 0.2609148);
				else lastSampleR = -0.2491717 + (lastSampleR * 0.7390851);
			}
			wasNegClipR = false;
			if (inputSampleR < -0.9549925859) {
				wasNegClipR = true;
				inputSampleR = -0.7058208 + (lastSampleR * 0.2609148);
			}
			intermediateR[spacing] = inputSampleR;
			inputSampleR = lastSampleR; // Latency is however many samples equals one 44.1k sample
			for (int x = spacing; x > 0; x--) intermediateR[x - 1] = intermediateR[x];
			lastSampleR = intermediateR[0]; // run a little buffer to handle this
			// end ClipOnly2 stereo as a little, compressed chunk that can be dropped into code

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
	bool wasPosClipL;
	bool wasNegClipL;
	double lastSampleR;
	double intermediateR[16];
	bool wasPosClipR;
	bool wasNegClipR; // Stereo ClipOnly2
					  // default stuff
};
} // namespace trnr
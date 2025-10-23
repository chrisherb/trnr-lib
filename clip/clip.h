#pragma once
#include <cmath>
#include <cstdlib>

namespace trnr {
// clipper based on ClipOnly2 by Chris Johnson (MIT License)
struct clip {
	double samplerate;

	double last_sample_l;
	double intermediate_l[16];
	bool was_pos_clip_l;
	bool was_neg_clip_l;

	double last_sample_r;
	double intermediate_r[16];
	bool was_pos_clip_r;
	bool was_neg_clip_r;
};

inline void clip_init(clip& c, double _samplerate)
{
	c.samplerate = 44100;

	c.last_sample_l = 0.0;
	c.was_pos_clip_l = false;
	c.was_neg_clip_l = false;
	c.last_sample_r = 0.0;
	c.was_pos_clip_r = false;
	c.was_neg_clip_r = false;

	for (int x = 0; x < 16; x++) {
		c.intermediate_l[x] = 0.0;
		c.intermediate_r[x] = 0.0;
	}
}

template <typename t_sample>
inline void clip_process_block(clip& c, t_sample** inputs, t_sample** outputs, long sample_frames)
{
	t_sample* in1 = inputs[0];
	t_sample* in2 = inputs[1];
	t_sample* out1 = outputs[0];
	t_sample* out2 = outputs[1];

	double overallscale = 1.0;
	overallscale /= 44100.0;
	overallscale *= c.samplerate;

	int spacing = floor(overallscale); // should give us working basic scaling, usually 2 or 4
	if (spacing < 1) spacing = 1;
	if (spacing > 16) spacing = 16;

	while (--sample_frames >= 0) {
		double input_l = *in1;
		double input_r = *in2;

		// begin ClipOnly2 stereo as a little, compressed chunk that can be dropped into code
		if (input_l > 4.0) input_l = 4.0;
		if (input_l < -4.0) input_l = -4.0;
		if (c.was_pos_clip_l == true) { // current will be over
			if (input_l < c.last_sample_l) c.last_sample_l = 0.7058208 + (input_l * 0.2609148);
			else c.last_sample_l = 0.2491717 + (c.last_sample_l * 0.7390851);
		}
		c.was_pos_clip_l = false;
		if (input_l > 0.9549925859) {
			c.was_pos_clip_l = true;
			input_l = 0.7058208 + (c.last_sample_l * 0.2609148);
		}
		if (c.was_neg_clip_l == true) { // current will be -over
			if (input_l > c.last_sample_l) c.last_sample_l = -0.7058208 + (input_l * 0.2609148);
			else c.last_sample_l = -0.2491717 + (c.last_sample_l * 0.7390851);
		}
		c.was_neg_clip_l = false;
		if (input_l < -0.9549925859) {
			c.was_neg_clip_l = true;
			input_l = -0.7058208 + (c.last_sample_l * 0.2609148);
		}
		c.intermediate_l[spacing] = input_l;
		input_l = c.last_sample_l; // Latency is however many samples equals one 44.1k sample
		for (int x = spacing; x > 0; x--) c.intermediate_l[x - 1] = c.intermediate_l[x];
		c.last_sample_l = c.intermediate_l[0]; // run a little buffer to handle this

		if (input_r > 4.0) input_r = 4.0;
		if (input_r < -4.0) input_r = -4.0;
		if (c.was_pos_clip_r == true) { // current will be over
			if (input_r < c.last_sample_r) c.last_sample_r = 0.7058208 + (input_r * 0.2609148);
			else c.last_sample_r = 0.2491717 + (c.last_sample_r * 0.7390851);
		}
		c.was_pos_clip_r = false;
		if (input_r > 0.9549925859) {
			c.was_pos_clip_r = true;
			input_r = 0.7058208 + (c.last_sample_r * 0.2609148);
		}
		if (c.was_neg_clip_r == true) { // current will be -over
			if (input_r > c.last_sample_r) c.last_sample_r = -0.7058208 + (input_r * 0.2609148);
			else c.last_sample_r = -0.2491717 + (c.last_sample_r * 0.7390851);
		}
		c.was_neg_clip_r = false;
		if (input_r < -0.9549925859) {
			c.was_neg_clip_r = true;
			input_r = -0.7058208 + (c.last_sample_r * 0.2609148);
		}
		c.intermediate_r[spacing] = input_r;
		input_r = c.last_sample_r; // Latency is however many samples equals one 44.1k sample
		for (int x = spacing; x > 0; x--) c.intermediate_r[x - 1] = c.intermediate_r[x];
		c.last_sample_r = c.intermediate_r[0]; // run a little buffer to handle this
		// end ClipOnly2 stereo as a little, compressed chunk that can be dropped into code

		*out1 = input_l;
		*out2 = input_r;

		in1++;
		in2++;
		out1++;
		out2++;
	}
}
} // namespace trnr
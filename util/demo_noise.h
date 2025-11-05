#pragma once

#include <math.h>

namespace trnr {

struct demo_noise {
	double samplerate;
	int counter = 0;
	// initialize with sane defaults
	int noise_len_sec = 3;
	int pause_len_sec = 17;
	float noise_gain = 0.1f;
};

inline void demo_nose_init(demo_noise& d, double samplerate)
{
	d.samplerate = samplerate;
	d.counter = 0;
}

// overwrites the input buffer with noise in the specified time frame
inline void process_block(demo_noise& d, float** samples, int blocksize)
{
	int noise_len_samples = d.noise_len_sec * d.samplerate;
	int pause_len_samples = d.pause_len_sec * d.samplerate;
	int total_len_samples = noise_len_samples + pause_len_samples;

	for (int s = 0; s < blocksize; s++) {
		d.counter++;

		if (d.counter > total_len_samples) { d.counter = 0; }
		if (d.counter > pause_len_samples) {
			float r1 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
			float r2 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

			float noise = static_cast<float>(sqrt(-2.0 * log(r1)) * cos(2.0 * M_PI * r2));

			samples[0][s] = samples[1][s] = noise * d.noise_gain;
		}
	}
}
} // namespace trnr

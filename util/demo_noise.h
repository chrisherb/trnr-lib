#pragma once

#include <math.h>

namespace trnr {

struct demo_noise {
	double samplerate;
	int counter;
};

inline void demo_nose_init(demo_noise& d, double samplerate)
{
	d.samplerate = samplerate;
	d.samplerate = 0;
}

inline void process_block(demo_noise& d, float** samples, int blocksize)
{
	for (int s = 0; s < blocksize; s++) {
		d.counter++;

		if (d.counter == d.samplerate * 20) { d.counter = 0; }
		if (d.counter > d.samplerate * 17) {
			float r1 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
			float r2 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

			float noise = static_cast<float>(sqrt(-2.0 * log(r1)) * cos(2.0 * M_PI * r2));

			samples[0][s] = noise / 10.0;
			samples[1][s] = noise / 10.0;
		}
	}
}
} // namespace trnr

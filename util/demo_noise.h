#pragma once
#define _USE_MATH_DEFINES
#include <math.h>
#include <cstdlib>

namespace trnr {
template <typename t_sample>
class demo_noise {
public:
    void set_samplerate(double _samplerate) {
        samplerate = _samplerate;
    }

    void process_block(t_sample** samples, long sample_frames) {

        for (int s = 0; s < sample_frames; s++) {
            demo_counter++;

            if (demo_counter == samplerate * 20) {
                demo_counter = 0;
            }
            if (demo_counter > samplerate * 17) {
                t_sample r1 = static_cast<t_sample>(rand()) / static_cast<t_sample>(RAND_MAX);
                t_sample r2 = static_cast<t_sample>(rand()) / static_cast<t_sample>(RAND_MAX);

                t_sample noise = static_cast<t_sample>(sqrt(-2.0 * log(r1)) * cos(2.0 * M_PI * r2));

                samples[0][s] = noise / 10.0;
                samples[1][s] = noise / 10.0;
            }
        }
    }

private:
    double samplerate = 44100;
    int demo_counter = 0;
};
}
#pragma once
#include <array>

namespace trnr {
template <typename t_sample, unsigned int sample_size>
class averager {
public:
	averager() { samples.fill(0); }

	t_sample process_sample(t_sample& _sample)
	{
		t_sample sum = t_sample(0);

		for (unsigned int i = 0; i < sample_size; i++) {

			if (i < sample_size - 1) {
				// shift to the left
				samples[i] = samples[i + 1];
			} else {
				// put new sample last
				samples[i] = _sample;
			}

			sum += samples[i];
		}

		return sum / sample_size;
	}

private:
	std::array<t_sample, sample_size> samples;
};
} // namespace trnr
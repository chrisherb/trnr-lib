#pragma once

namespace trnr {
// folds the wave from -1 to 1
inline float fold(float& sample)
{
	while (sample > 1.0 || sample < -1.0) {

		if (sample > 1.0) {
			sample = 2.0 - sample;
		} else if (sample < -1.0) {
			sample = -2.0 - sample;
		}
	}
	return sample;
}

// folds the positive part of the wave independently from the negative part.
inline float fold_bipolar(float& sample)
{
	// fold positive values
	if (sample > 1.0) {
		sample = 2.0 - sample;

		if (sample < 0.0) { sample = -sample; }

		return fold(sample);
	}
	// fold negative values
	else if (sample < -1.0) {
		sample = -2.0 - sample;

		if (sample > 0.0) { sample = -sample; }

		return fold(sample);
	} else {
		return sample;
	}
}
}; // namespace trnr

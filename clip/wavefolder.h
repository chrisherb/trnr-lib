#pragma once

namespace trnr {
class wavefolder {
public:
	float amount = 1.f;

	void process_sample(float& _sample)
	{
		if (amount > 1.f) { _sample = fold(_sample * amount); }
	}

private:
	// folds the wave from -1 to 1
	float fold(float _sample)
	{
		while (_sample > 1.0 || _sample < -1.0) {

			if (_sample > 1.0) {
				_sample = 2.0 - _sample;
			} else if (_sample < -1.0) {
				_sample = -2.0 - _sample;
			}
		}

		return _sample;
	}

	// folds the positive part of the wave independently from the negative part.
	float fold_bipolar(float _sample)
	{
		// fold positive values
		if (_sample > 1.0) {
			_sample = 2.0 - _sample;

			if (_sample < 0.0) { _sample = -_sample; }

			return fold(_sample);
		}
		// fold negative values
		else if (_sample < -1.0) {
			_sample = -2.0 - _sample;

			if (_sample > 0.0) { _sample = -_sample; }

			return fold(_sample);
		} else {
			return _sample;
		}
	}
};
}; // namespace trnr
#pragma once

namespace trnr {
class wavefolder {
public:
	float amount = 1.f;

	float process_sample(float& _sample)
	{
		if (amount > 1.f) { return fold(_sample * amount); }

        return 0.f;
	}

private:
	float fold(float _sample)
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
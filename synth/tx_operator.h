#pragma once
#include "tx_envelope.h"
#include "tx_sineosc.h"

namespace trnr {
class tx_operator {
public:
	tx_operator()
		: ratio {1}
		, amplitude {1.0f}
	{
	}

	tx_envelope envelope;
	tx_sineosc oscillator;
	float ratio;
	float amplitude;

	float process_sample(const bool& gate, const bool& trigger, const float& frequency, const float& velocity,
						 const float& pm = 0)
	{

		float env = envelope.process_sample(gate, trigger);

		// drifts and sounds better!
		if (envelope.is_busy()) {
			double osc = oscillator.process_sample(trigger, frequency, pm);
			return osc * env * velocity;
		} else {
			return 0.;
		}
	}

	void set_samplerate(double samplerate)
	{
		this->envelope.set_samplerate(samplerate);
		this->oscillator.set_samplerate(samplerate);
	}
};
} // namespace trnr

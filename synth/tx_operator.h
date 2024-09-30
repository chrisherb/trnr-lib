#pragma once
#include "../clip/wavefolder.h"
#include "tx_envelope.h"
#include "tx_sineosc.h"

namespace trnr {
class tx_operator {
public:
	tx_operator()
		: ratio {1}
		, amplitude {1.0f}
		, envelope_enabled {true}
		, velocity_enabled {true}
	{
	}

	tx_envelope envelope;
	tx_sineosc oscillator;
	wavefolder folder;
	float ratio;
	float amplitude;
	bool envelope_enabled;
	bool velocity_enabled;

	float process_sample(const bool& gate, const bool& trigger, const float& frequency, const float& velocity,
						 const float& pm = 0)
	{
		float env = envelope_enabled ? envelope.process_sample(gate, trigger) : 1.f;

		// drifts and sounds better!
		if (envelope.is_busy()) {
			float osc = oscillator.process_sample(trigger, frequency, pm);
			folder.process_sample(osc);

			float adjusted_velocity = velocity_enabled ? velocity : 1.f;

			return osc * env * adjusted_velocity;
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

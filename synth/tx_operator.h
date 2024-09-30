#pragma once
#include "../clip/wavefolder.h"
#include "../util/averager.h"
#include "tx_envelope.h"
#include "tx_sineosc.h"

namespace trnr {
class tx_operator {
public:
	tx_operator()
		: ratio {1}
		, amplitude {1.0f}
		, wavefolding_amt {1.0f}
	{
	}

	tx_envelope envelope;
	tx_sineosc oscillator;
	float ratio;
	float amplitude;
	float wavefolding_amt;

	float process_sample(const bool& gate, const bool& trigger, const float& frequency, const float& velocity,
						 const float& pm = 0)
	{
		float env = envelope.process_sample(gate, trigger);
		float avg = m_averager.process_sample(wavefolding_amt);
		m_wavefolder.amount = avg;

		// drifts and sounds better!
		if (envelope.is_busy()) {
			float osc = oscillator.process_sample(trigger, frequency, pm);
			m_wavefolder.process_sample(osc);
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

private:
	averager<float, 1000> m_averager;
	wavefolder m_wavefolder;
};
} // namespace trnr

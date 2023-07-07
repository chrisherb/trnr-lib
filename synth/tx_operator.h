#pragma once
#include "tx_sineosc.h"
#include "tx_envelope.h"

namespace trnr::lib::synth {
class tx_operator {
public:
    tx_operator(double samplerate)
        : ratio { 1 }
        , amplitude { 1.0f }
        , envelope(samplerate)
        , oscillator(samplerate)
    {
    }

    tx_envelope envelope;
    tx_sineosc oscillator;
    float ratio;
    float amplitude;

    float process_sample(const bool& gate, const bool& trigger, const float& frequency, const float& velocity, const float& pm = 0) {

        float env = envelope.process_sample(gate, trigger);

        // drifts and sounds better!
        if (envelope.is_busy()) {
            double osc = oscillator.process_sample(trigger, frequency, pm);
            return osc * env * velocity;
        } else {
            return 0.;
        }
    }

    void set_samplerate(double samplerate) {
        this->envelope.set_samplerate(samplerate);
        this->oscillator.set_samplerate(samplerate);
    }
};
}

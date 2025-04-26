#include <cmath>
namespace trnr {
    enum tx_parameter {
        BIT_RESOLUTION = 0,
        FEEDBACKOSC_PHASE_RESOLUTION,
        FEEDBACK,
        ALGORITHM,

        PITCH_ENVELOPE_AMOUNT,
        PITCH_ENVELOPE_SKIP_SUSTAIN,
        PITCH_ENVELOPE_ATTACK1_RATE,
        PITCH_ENVELOPE_ATTACK1_LEVEL,
        PITCH_ENVELOPE_ATTACK2_RATE,
        PITCH_ENVELOPE_HOLD_RATE,
        PITCH_ENVELOPE_DECAY1_RATE,
        PITCH_ENVELOPE_DECAY1_LEVEL,
        PITCH_ENVELOPE_DECAY2_RATE,
        PITCH_ENVELOPE_SUSTAIN_LEVEL,
        PITCH_ENVELOPE_RELEASE1_RATE,
        PITCH_ENVELOPE_RELEASE1_LEVEL,
        PITCH_ENVELOPE_RELEASE2_RATE,

        OP1_RATIO,
        OP1_AMPLITUDE,
        OP1_PHASE_RESOLUTION,
        OP1_ENVELOPE_SKIP_SUSTAIN,
        OP1_ENVELOPE_ATTACK1_RATE,
        OP1_ENVELOPE_ATTACK1_LEVEL,
        OP1_ENVELOPE_ATTACK2_RATE,
        OP1_ENVELOPE_HOLD_RATE,
        OP1_ENVELOPE_DECAY1_RATE,
        OP1_ENVELOPE_DECAY1_LEVEL,
        OP1_ENVELOPE_DECAY2_RATE,
        OP1_ENVELOPE_SUSTAIN_LEVEL,
        OP1_ENVELOPE_RELEASE1_RATE,
        OP1_ENVELOPE_RELEASE1_LEVEL,
        OP1_ENVELOPE_RELEASE2_RATE,

        OP2_RATIO,
        OP2_AMPLITUDE,
        OP2_PHASE_RESOLUTION,
        OP2_ENVELOPE_SKIP_SUSTAIN,
        OP2_ENVELOPE_ATTACK1_RATE,
        OP2_ENVELOPE_ATTACK1_LEVEL,
        OP2_ENVELOPE_ATTACK2_RATE,
        OP2_ENVELOPE_HOLD_RATE,
        OP2_ENVELOPE_DECAY1_RATE,
        OP2_ENVELOPE_DECAY1_LEVEL,
        OP2_ENVELOPE_DECAY2_RATE,
        OP2_ENVELOPE_SUSTAIN_LEVEL,
        OP2_ENVELOPE_RELEASE1_RATE,
        OP2_ENVELOPE_RELEASE1_LEVEL,
        OP2_ENVELOPE_RELEASE2_RATE,

        OP3_RATIO,
        OP3_AMPLITUDE,
        OP3_PHASE_RESOLUTION,
        OP3_ENVELOPE_SKIP_SUSTAIN,
        OP3_ENVELOPE_ATTACK1_RATE,
        OP3_ENVELOPE_ATTACK1_LEVEL,
        OP3_ENVELOPE_ATTACK2_RATE,
        OP3_ENVELOPE_HOLD_RATE,
        OP3_ENVELOPE_DECAY1_RATE,
        OP3_ENVELOPE_DECAY1_LEVEL,
        OP3_ENVELOPE_DECAY2_RATE,
        OP3_ENVELOPE_SUSTAIN_LEVEL,
        OP3_ENVELOPE_RELEASE1_RATE,
        OP3_ENVELOPE_RELEASE1_LEVEL,
        OP3_ENVELOPE_RELEASE2_RATE,
    };

    struct tx_parameter_mapping {
        float range_min;
        float range_max;
        float exponent;
        tx_parameter parameter;
        
        tx_parameter_mapping(float _range_min, float _range_max, float _exponent, tx_parameter _parameter)
            : range_min(_range_min), range_max(_range_max), exponent(_exponent), parameter(_parameter)
        {}

        float apply(float _input) const
        {
            if (range_min == range_max && exponent == 1.f) return _input;

            return powf(_input, exponent) * (range_max - range_min) + range_min;
        }
    };
}
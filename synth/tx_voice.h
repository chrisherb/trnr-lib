#pragma once
#include "../util/audio_math.h"
#include "ivoice.h"
#include "tx_envelope.h"
#include "tx_operator.h"
#include "tx_sineosc.h"

namespace trnr {

class tx_voice : public ivoice {
public:
    tx_voice()
        : algorithm { 0 }
        , pitch_env_amt { 0.f }
        , feedback_amt { 0.f }
        , bit_resolution(12.f)
    {
    }

    bool gate = false;
    bool trigger = false;
    int midi_note = 0;
    float velocity = 1.f;
    float additional_pitch_mod = 0.f; // modulates pitch in frequency

    int algorithm;
    float pitch_env_amt;
    float feedback_amt;
    float bit_resolution;
    tx_sineosc feedback_osc;
    tx_envelope pitch_env;
    tx_operator op1;
    tx_operator op2;
    tx_operator op3;

    void note_on(int _note, float _velocity) override
    {
        this->gate = true;
        this->trigger = true;
        midi_note = _note;
        velocity = _velocity;
    }

    void note_off() override
    {
        this->gate = false;
    }

    // modulates the pitch in semitones
    void modulate_pitch(float _pitch) override
    {
        this->pitch_mod = _pitch;
    }

    float process_sample() override
    {
        float pitch_env_signal = pitch_env.process_sample(gate, trigger) * pitch_env_amt;
        float pitched_freq = midi_to_frequency(midi_note + pitch_mod + additional_pitch_mod) + pitch_env_signal;

        float output = 0.f;

        // mix operator signals according to selected algorithm
        switch (algorithm) {
        case 0:
            output = calc_algo1(pitched_freq);
            break;
        case 1:
            output = calc_algo2(pitched_freq);
            break;
        case 2:
            output = calc_algo3(pitched_freq);
            break;
        case 3:
            output = calc_algo4(pitched_freq);
            break;
        default:
            output = calc_algo1(pitched_freq);
            break;
        }

        // reset trigger
        trigger = false;

        return redux(output, bit_resolution);
    }

    bool is_busy() override { return gate || op1.envelope.is_busy() || op2.envelope.is_busy() || op3.envelope.is_busy(); }

    void set_samplerate(double samplerate) override
    {
        pitch_env.set_samplerate(samplerate);
        feedback_osc.set_samplerate(samplerate);
        op1.set_samplerate(samplerate);
        op2.set_samplerate(samplerate);
        op3.set_samplerate(samplerate);
    }

    void set_phase_reset(bool phase_reset)
    {
        op1.oscillator.phase_reset = phase_reset;
        op2.oscillator.phase_reset = phase_reset;
        op3.oscillator.phase_reset = phase_reset;
        feedback_osc.phase_reset = phase_reset;
    }

private:
    const float MOD_INDEX_COEFF = 4.f;
    float pitch_mod = 0.f; // modulates pitch in semi-tones

    float calc_algo1(const float frequency)
    {
        float fb_freq = frequency * op3.ratio;
        float fb_mod_index = (feedback_amt * MOD_INDEX_COEFF);
        float fb_signal = feedback_osc.process_sample(trigger, fb_freq) * fb_mod_index;

        float op3_Freq = frequency * op3.ratio;
        float op3_mod_index = (op3.amplitude * MOD_INDEX_COEFF);
        float op3_signal = op3.process_sample(gate, trigger, op3_Freq, velocity, fb_signal) * op3_mod_index;

        float op2_freq = frequency * op2.ratio;
        float op2_mod_index = (op2.amplitude * MOD_INDEX_COEFF);
        float op2_signal = op2.process_sample(gate, trigger, op2_freq, velocity, op3_signal) * op2_mod_index;

        float op1_freq = frequency * op1.ratio;
        return op1.process_sample(gate, trigger, op1_freq, velocity, op2_signal) * op1.amplitude;
    }

    float calc_algo2(const float frequency)
    {
        float fb_freq = frequency * op3.ratio;
        float fb_mod_index = (feedback_amt * MOD_INDEX_COEFF);
        float fb_signal = feedback_osc.process_sample(trigger, fb_freq) * fb_mod_index;

        float op3_freq = frequency * op3.ratio;
        float op3_signal = op3.process_sample(gate, trigger, op3_freq, velocity, fb_signal) * op3.amplitude;

        float op2_freq = frequency * op2.ratio;
        float op2_mod_index = (op2.amplitude * MOD_INDEX_COEFF);
        float op2_signal = op2.process_sample(gate, trigger, op2_freq, velocity) * op2_mod_index;

        float op1_freq = frequency * op1.ratio;
        float op1_signal = op1.process_sample(gate, trigger, op1_freq, velocity, op2_signal) * op1.amplitude;

        return op1_signal + op3_signal;
    }

    float calc_algo3(const float frequency)
    {
        float fb_freq = frequency * op3.ratio;
        float fb_mod_index = (feedback_amt * MOD_INDEX_COEFF);
        float fb_signal = feedback_osc.process_sample(trigger, fb_freq) * fb_mod_index;

        float op3_freq = frequency * op3.ratio;
        float op3_signal = op3.process_sample(gate, trigger, op3_freq, velocity, fb_signal) * op3.amplitude;

        float op2_freq = frequency * op2.ratio;
        float op2_signal = op2.process_sample(gate, trigger, op2_freq, velocity) * op2.amplitude;

        float op1_freq = frequency * op1.ratio;
        float op1_signal = op1.process_sample(gate, trigger, op1_freq, velocity) * op1.amplitude;

        return op1_signal + op2_signal + op3_signal;
    }

    float calc_algo4(const float frequency)
    {
        float fb_freq = frequency * op3.ratio;
        float fb_mod_index = (feedback_amt * MOD_INDEX_COEFF);
        float fb_signal = feedback_osc.process_sample(trigger, fb_freq) * fb_mod_index;

        float op3_freq = frequency * op3.ratio;
        float op3_mod_index = (op3.amplitude * MOD_INDEX_COEFF);
        float op3_signal = op3.process_sample(gate, trigger, op3_freq, velocity, fb_signal) * op3_mod_index;

        float op2_freq = frequency * op2.ratio;
        float op2_mod_index = (op2.amplitude * MOD_INDEX_COEFF);
        float op2_signal = op2.process_sample(gate, trigger, op2_freq, velocity) * op2_mod_index;

        float op1_freq = frequency * op1.ratio;
        return op1.process_sample(gate, trigger, op1_freq, velocity, op2_signal + op3_signal) * op1.amplitude;
    }

    float redux(float& value, float resolution)
    {
        float res = powf(2, resolution);
        value = roundf(value * res) / res;

        return value;
    }
};
}
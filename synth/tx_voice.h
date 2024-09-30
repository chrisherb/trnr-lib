#pragma once
#include "../util/audio_math.h"
#include "ivoice.h"
#include "tx_envelope.h"
#include "tx_operator.h"
#include "tx_sineosc.h"

namespace trnr {

enum mod_dest {
	out = 0,
	fm,
};

template <typename t_sample>
class tx_voice : public ivoice<t_sample> {
public:
	tx_voice()
		: pitch_env_amt {0.f}
		, feedback_amt {0.f}
		, bit_resolution(12.f)
	{
	}

	bool gate = false;
	bool trigger = false;
	int midi_note = 0;
	float velocity = 1.f;
	float additional_pitch_mod = 0.f; // modulates pitch in frequency

	mod_dest op3_dest;
	mod_dest op2_dest;

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

	void note_off() override { this->gate = false; }

	// modulates the pitch in semitones
	void modulate_pitch(float _pitch) override { this->pitch_mod = _pitch; }

	void process_samples(t_sample** _outputs, int _start_index, int _block_size) override
	{
		float frequency = midi_to_frequency(midi_note + pitch_mod + additional_pitch_mod);

		for (int s = _start_index; s < _start_index + _block_size; s++) {

			float pitch_env_signal = pitch_env.process_sample(gate, trigger) * pitch_env_amt;
			float pitched_freq = frequency + pitch_env_signal;

			float op3_signal = process_op3(pitched_freq);

			float op2_pm = op3_dest == fm ? op3_signal : 0.f;
			float op2_signal = process_op2(pitched_freq, op2_pm);

			float op1_pm = op2_dest == fm ? op2_signal : 0.f;
			float op1_signal = process_op1(pitched_freq, op1_pm);

			float signal_mix = op1_signal;
			if (op3_dest == out) { signal_mix += op3_signal; }
			if (op2_dest == out) { signal_mix += op2_signal; }

			// reset trigger
			trigger = false;

			redux(signal_mix, bit_resolution);

			_outputs[0][s] += signal_mix / 3.;
			_outputs[1][s] = _outputs[0][s];
		}
	}

	bool is_busy() override
	{
		return gate || op1.envelope.is_busy() || op2.envelope.is_busy() || op3.envelope.is_busy();
	}

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

	float process_op3(const float frequency)
	{
		float fb_freq = frequency * op3.ratio;
		float fb_mod_index = (feedback_amt * MOD_INDEX_COEFF);
		float fb_signal = feedback_osc.process_sample(trigger, fb_freq) * fb_mod_index;

		float op3_Freq = frequency * op3.ratio;
		float op3_mod_index = (op3.amplitude * MOD_INDEX_COEFF);
		return op3.process_sample(gate, trigger, op3_Freq, velocity, fb_signal) * op3_mod_index;
	}

	float process_op2(const float frequency, const float phase_mod = 0.f)
	{
		float op2_freq = frequency * op2.ratio;
		float op2_mod_index = (op2.amplitude * MOD_INDEX_COEFF);
		return op2.process_sample(gate, trigger, op2_freq, velocity, phase_mod) * op2_mod_index;
	}

	float process_op1(const float frequency, const float phase_mod = 0.f)
	{
		float op1_freq = frequency * op1.ratio;
		return op1.process_sample(gate, trigger, op1_freq, velocity, phase_mod) * op1.amplitude;
	}

	float redux(float& value, float resolution)
	{
		float res = powf(2, resolution);
		value = roundf(value * res) / res;

		return value;
	}
};
} // namespace trnr
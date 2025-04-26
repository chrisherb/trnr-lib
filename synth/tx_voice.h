#pragma once
#include "../util/audio_math.h"
#include "ivoice.h"
#include "tx_envelope.h"
#include "tx_operator.h"
#include "tx_sineosc.h"
#include "tx_parameter_mapping.h"

namespace trnr {

enum mod_dest {
	mod_dest_out = 0,
	mod_dest_fm,
	mod_dest_am
};

template <typename t_sample>
class tx_voice : public ivoice<t_sample> {
public:
	tx_voice()
		: pitch_env_amt {0.f}
		, feedback_amt {0.f}
		, bit_resolution(12.f)
	{
		set_glide_time(0.f);
	}

	bool gate = false;
	bool trigger = false;
	int midi_note = 0;
	float velocity = 1.f;
	float additional_pitch_mod = 0.f; // modulates pitch in frequency

	mod_dest op2_dest = mod_dest_fm;
	mod_dest op3_dest = mod_dest_fm;

	float pitch_env_amt;
	float feedback_amt;
	float bit_resolution;
	tx_sineosc feedback_osc;
	tx_envelope pitch_env;
	tx_operator op1;
	tx_operator op2;
	tx_operator op3;

	void set_glide_time(float time_ms) { glide = 1 - exp(-1.0 / (time_ms * samplerate / 1000.f)); }

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
		target_frequency = midi_to_frequency(midi_note + pitch_mod + additional_pitch_mod);

		for (int s = _start_index; s < _start_index + _block_size; s++) {

			// calculate moving average for portamento
			current_frequency = (1 - glide) * current_frequency + glide * target_frequency;

			float pitch_env_signal = pitch_env.process_sample(gate, trigger) * pitch_env_amt;
			float pitched_freq = current_frequency + pitch_env_signal;

			float signal = process_operators(pitched_freq);

			// reset trigger
			trigger = false;

			redux(signal, bit_resolution);

			_outputs[0][s] += signal / 3.;
			_outputs[1][s] = _outputs[0][s];
		}
	}

	bool is_busy() override
	{
		return gate || op1.envelope.is_busy() || op2.envelope.is_busy() || op3.envelope.is_busy();
	}

	void set_samplerate(double _samplerate) override
	{
		samplerate = _samplerate;
		pitch_env.set_samplerate(_samplerate);
		feedback_osc.set_samplerate(_samplerate);
		op1.set_samplerate(_samplerate);
		op2.set_samplerate(_samplerate);
		op3.set_samplerate(_samplerate);
	}

	void set_phase_reset(bool phase_reset)
	{
		op1.oscillator.phase_reset = phase_reset;
		op2.oscillator.phase_reset = phase_reset;
		op3.oscillator.phase_reset = phase_reset;
		feedback_osc.phase_reset = phase_reset;
	}

	void update_parameters(float _value, const std::vector<tx_parameter_mapping>& _mappings)
	{
		for (const tx_parameter_mapping& mapping : _mappings) {
			float normalized = mapping.apply(_value);
			map_parameter(mapping, normalized);
		}
	}

private:
	double samplerate;
	const float MOD_INDEX_COEFF = 4.f;
	float pitch_mod = 0.f; // modulates pitch in semi-tones

	float current_frequency;
	float target_frequency;
	float glide;

	float process_op3(const float frequency)
	{
		float fb_freq = frequency * op3.ratio;
		float fb_mod_index = feedback_amt * MOD_INDEX_COEFF;
		float fb_signal = feedback_osc.process_sample(trigger, fb_freq) * fb_mod_index;

		float op3_Freq = frequency * op3.ratio;
		return op3.process_sample(gate, trigger, op3_Freq, velocity, fb_signal) * op3.amplitude;
	}

	float process_op2(const float frequency, const float modulator)
	{
		// if patched, op3 modulates the phase of op2
		float pm = op3_dest == mod_dest_fm ? modulator : 0.f;

		float adjusted_freq = frequency * op2.ratio;
		float signal = op2.process_sample(gate, trigger, adjusted_freq, velocity, pm * MOD_INDEX_COEFF) * op2.amplitude;

		// if patched, op3 modulated the amplitude of op2
		if (op3_dest == mod_dest_am) ring_mod(signal, modulator, op3.amplitude);

		return signal;
	}

	float process_op1(const float frequency, const float modulator)
	{
		// if patched, op2 modulates the phase of op1
		float pm = op2_dest == mod_dest_fm ? modulator : 0.f;

		float op1_freq = frequency * op1.ratio;
		float signal = op1.process_sample(gate, trigger, op1_freq, velocity, pm * MOD_INDEX_COEFF) * op1.amplitude;

		// if patched, op2 modulates the amplitude of op1
		if (op2_dest == mod_dest_am) ring_mod(signal, modulator, op2.amplitude);

		return signal;
	}

	float process_operators(float frequency)
	{
		float op3_signal = process_op3(frequency);

		float op2_signal = process_op2(frequency, op3_signal);

		float op1_signal = process_op1(frequency, op2_signal);

		float signal_mix = op1_signal;
		if (op3_dest == mod_dest_out) { signal_mix += op3_signal; }
		if (op2_dest == mod_dest_out) { signal_mix += op2_signal; }

		return signal_mix;
	}

	void ring_mod(float& carrier, float modulator, float blend)
	{
		float dry_lvl = 1.f - blend;
		float wet_lvl = blend;

		float dry_signal = carrier;
		float wet_signal = carrier * modulator * 2.0f;

		carrier = dry_lvl * dry_signal + wet_lvl * wet_signal;
	}

	float redux(float& value, float resolution)
	{
		float res = powf(2, resolution);
		value = roundf(value * res) / res;

		return value;
	}

	void map_parameter(const tx_parameter_mapping& _mapping, const float _value)
	{
		switch (_mapping.parameter) {
			case tx_parameter::BIT_RESOLUTION:
				bit_resolution = _value;
				break;
			case tx_parameter::FEEDBACKOSC_PHASE_RESOLUTION:
				feedback_osc.set_phase_resolution(_value);
				break;
			case tx_parameter::FEEDBACK:
				feedback_amt = _value;
				break;
			case tx_parameter::ALGORITHM:
				algorithm = _value;
				break;
			case tx_parameter::PITCH_ENVELOPE_AMOUNT:
				pitch_env_amt = _value;
				break;
			case tx_parameter::PITCH_ENVELOPE_SKIP_SUSTAIN:
				pitch_env.skip_sustain = _value;
				break;
			case tx_parameter::PITCH_ENVELOPE_ATTACK1_RATE:
				pitch_env.attack1_rate = _value;
				break;
			case tx_parameter::PITCH_ENVELOPE_ATTACK1_LEVEL:
				pitch_env.attack1_level = _value;
				break;
			case tx_parameter::PITCH_ENVELOPE_ATTACK2_RATE:
				pitch_env.attack2_rate = _value;
				break;
			case tx_parameter::PITCH_ENVELOPE_HOLD_RATE:
				pitch_env.hold_rate = _value;
				break;
			case tx_parameter::PITCH_ENVELOPE_DECAY1_RATE:
				pitch_env.decay1_rate = _value;
				break;
			case tx_parameter::PITCH_ENVELOPE_DECAY1_LEVEL:
				pitch_env.decay1_level = _value;
				break;
			case tx_parameter::PITCH_ENVELOPE_DECAY2_RATE:
				pitch_env.decay2_rate = _value;
				break;
			case tx_parameter::PITCH_ENVELOPE_SUSTAIN_LEVEL:
				pitch_env.sustain_level = _value;
				break;
			case tx_parameter::PITCH_ENVELOPE_RELEASE1_RATE:
				pitch_env.release1_rate = _value;
				break;
			case tx_parameter::PITCH_ENVELOPE_RELEASE1_LEVEL:
				pitch_env.release1_level = _value;
				break;
			case tx_parameter::PITCH_ENVELOPE_RELEASE2_RATE:
				pitch_env.release2_rate = _value;
				break;
			case tx_parameter::OP1_RATIO:
				op1.ratio = _value;
				break;
			case tx_parameter::OP1_AMPLITUDE:
				op1.amplitude = _value;
				break;
			case tx_parameter::OP1_PHASE_RESOLUTION:
				op1.oscillator.set_phase_resolution(_value);
				break;
			case tx_parameter::OP1_ENVELOPE_SKIP_SUSTAIN:
				op1.envelope.skip_sustain = _value;
				break;
			case tx_parameter::OP1_ENVELOPE_ATTACK1_RATE:
				op1.envelope.attack1_rate = _value;
				break;
			case tx_parameter::OP1_ENVELOPE_ATTACK1_LEVEL:
				op1.envelope.attack1_level = _value;
				break;
			case tx_parameter::OP1_ENVELOPE_ATTACK2_RATE:
				op1.envelope.attack2_rate = _value;
				break;
			case tx_parameter::OP1_ENVELOPE_HOLD_RATE:
				op1.envelope.hold_rate = _value;
				break;
			case tx_parameter::OP1_ENVELOPE_DECAY1_RATE:
				op1.envelope.decay1_rate = _value;
				break;
			case tx_parameter::OP1_ENVELOPE_DECAY1_LEVEL:
				op1.envelope.decay1_level = _value;
				break;
			case tx_parameter::OP1_ENVELOPE_DECAY2_RATE:
				op1.envelope.decay2_rate = _value;
				break;
			case tx_parameter::OP1_ENVELOPE_SUSTAIN_LEVEL:
				op1.envelope.sustain_level = _value;
				break;
			case tx_parameter::OP1_ENVELOPE_RELEASE1_RATE:
				op1.envelope.release1_rate = _value;
				break;
			case tx_parameter::OP1_ENVELOPE_RELEASE1_LEVEL:
				op1.envelope.release1_level = _value;
				break;
			case tx_parameter::OP1_ENVELOPE_RELEASE2_RATE:
				op1.envelope.release2_rate = _value;
				break;
			case tx_parameter::OP2_RATIO:
				op2.ratio = _value;
				break;
			case tx_parameter::OP2_AMPLITUDE:
				op2.amplitude = _value;
				break;
			case tx_parameter::OP2_PHASE_RESOLUTION:
				op2.oscillator.set_phase_resolution(_value);
				break;
			case tx_parameter::OP2_ENVELOPE_SKIP_SUSTAIN:
				op2.envelope.skip_sustain = _value;
				break;
			case tx_parameter::OP2_ENVELOPE_ATTACK1_RATE:
				op2.envelope.attack1_rate = _value;
				break;
			case tx_parameter::OP2_ENVELOPE_ATTACK1_LEVEL:
				op2.envelope.attack1_level = _value;
				break;
			case tx_parameter::OP2_ENVELOPE_ATTACK2_RATE:
				op2.envelope.attack2_rate = _value;
				break;
			case tx_parameter::OP2_ENVELOPE_HOLD_RATE:
				op2.envelope.hold_rate = _value;
				break;
			case tx_parameter::OP2_ENVELOPE_DECAY1_RATE:
				op2.envelope.decay1_rate = _value;
				break;
			case tx_parameter::OP2_ENVELOPE_DECAY1_LEVEL:
				op2.envelope.decay1_level = _value;
				break;
			case tx_parameter::OP2_ENVELOPE_DECAY2_RATE:
				op2.envelope.decay2_rate = _value;
				break;
			case tx_parameter::OP2_ENVELOPE_SUSTAIN_LEVEL:
				op2.envelope.sustain_level = _value;
				break;
			case tx_parameter::OP2_ENVELOPE_RELEASE1_RATE:
				op2.envelope.release1_rate = _value;
				break;
			case tx_parameter::OP2_ENVELOPE_RELEASE1_LEVEL:
				op2.envelope.release1_level = _value;
				break;
			case tx_parameter::OP2_ENVELOPE_RELEASE2_RATE:
				op2.envelope.release2_rate = _value;
				break;
			case tx_parameter::OP3_RATIO:
				op3.ratio = _value;
				break;
			case tx_parameter::OP3_AMPLITUDE:
				op3.amplitude = _value;
				break;
			case tx_parameter::OP3_PHASE_RESOLUTION:
				op3.oscillator.set_phase_resolution(_value);
				break;
			case tx_parameter::OP3_ENVELOPE_SKIP_SUSTAIN:
				op3.envelope.skip_sustain = _value;
				break;
			case tx_parameter::OP3_ENVELOPE_ATTACK1_RATE:
				op3.envelope.attack1_rate = _value;
				break;
			case tx_parameter::OP3_ENVELOPE_ATTACK1_LEVEL:
				op3.envelope.attack1_level = _value;
				break;
			case tx_parameter::OP3_ENVELOPE_ATTACK2_RATE:
				op3.envelope.attack2_rate = _value;
				break;
			case tx_parameter::OP3_ENVELOPE_HOLD_RATE:
				op3.envelope.hold_rate = _value;
				break;
			case tx_parameter::OP3_ENVELOPE_DECAY1_RATE:
				op3.envelope.decay1_rate = _value;
				break;
			case tx_parameter::OP3_ENVELOPE_DECAY1_LEVEL:
				op3.envelope.decay1_level = _value;
				break;
			case tx_parameter::OP3_ENVELOPE_DECAY2_RATE:
				op3.envelope.decay2_rate = _value;
				break;
			case tx_parameter::OP3_ENVELOPE_SUSTAIN_LEVEL:
				op3.envelope.sustain_level = _value;
				break;
			case tx_parameter::OP3_ENVELOPE_RELEASE1_RATE:
				op3.envelope.release1_rate = _value;
				break;
			case tx_parameter::OP3_ENVELOPE_RELEASE1_LEVEL:
				op3.envelope.release1_level = _value;
				break;
			case tx_parameter::OP3_ENVELOPE_RELEASE2_RATE:
				op3.envelope.release2_rate = _value;
				break;
		}
	}
};
} // namespace trnr
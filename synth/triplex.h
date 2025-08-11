#pragma once
#include "audio_buffer.h"
#include "audio_math.h"
#include "voice_allocator.h"
#include <cmath>
#include <random>

namespace trnr {

//////////////
// SINE OSC //
//////////////

struct tx_sineosc {
	bool phase_reset;
	double samplerate;
	float phase_resolution;
	float phase;
	float history;
};

inline void tx_randomize_phase(float& phase)
{
	std::random_device random;
	std::mt19937 gen(random());
	std::uniform_real_distribution<> dis(0.0, 1.0);
	phase = dis(gen);
}

inline void tx_sineosc_init(tx_sineosc& s, double samplerate)
{
	s.phase_reset = false;
	s.samplerate = samplerate;
	s.phase_resolution = 16.f;
	s.phase = 0.f;
	s.history = 0.f;

	tx_randomize_phase(s.phase);
}

inline float tx_wrap(float& phase)
{
	while (phase < 0.) phase += 1.;
	while (phase >= 1.) phase -= 1.;
	return phase;
}

inline float tx_sineosc_process_sample(tx_sineosc& s, bool trigger, float frequency, float phase_modulation = 0.f)
{
	if (trigger) {
		if (s.phase_reset) s.phase = 0.f;
		else tx_randomize_phase(s.phase);
	}

	float lookup_phase = s.phase + phase_modulation;
	tx_wrap(lookup_phase);
	s.phase += frequency / s.samplerate;
	tx_wrap(s.phase);

	// redux
	s.phase = static_cast<int>(s.phase * s.phase_resolution) / s.phase_resolution;

	// x is scaled 0<=x<4096
	float output;
	float x = lookup_phase;
	const float a = -0.40319426317E-08;
	const float b = 0.21683205691E+03;
	const float c = 0.28463350538E-04;
	const float d = -0.30774648337E-02;
	float y;

	bool negate = false;
	if (x > 2048) {
		negate = true;
		x -= 2048;
	}
	if (x > 1024) x = 2048 - x;
	y = (a + x) / (b + c * x * x) + d * x;
	if (negate) output = (-y);
	else output = y;

	// filter
	output = 0.5 * (output + s.history);
	s.history = output;

	return output;
}

//////////////
// ENVELOPE //
//////////////

enum tx_env_state {
	idle = 0,
	attack1,
	attack2,
	hold,
	decay1,
	decay2,
	sustain,
	release1,
	release2
};

struct tx_envelope {
	tx_env_state state = idle;
	float attack1_rate = 0;
	float attack1_level = 0;
	float attack2_rate = 0;
	float hold_rate = 0;
	float decay1_rate = 0;
	float decay1_level = 0;
	float decay2_rate = 0;
	float sustain_level = 0;
	float release1_rate = 0;
	float release1_level = 0;
	float release2_rate = 0;
	bool skip_sustain = false;

	double samplerate = 44100.;
	size_t phase = 0;
	float level = 0.f;
	float start_level = 0.f;
	float h1 = 0.f;
	float h2 = 0.f;
	float h3 = 0.f;
	bool retrigger;
};

inline void tx_envelope_init(tx_envelope& e, double samplerate, bool retrigger = false)
{
	e.samplerate = samplerate;
	e.retrigger = retrigger;
}

inline size_t tx_mtos(float ms, double samplerate) { return static_cast<size_t>(ms * samplerate / 1000.f); }

inline float tx_lerp(float x1, float y1, float x2, float y2, float x)
{
	return y1 + (((x - x1) * (y2 - y1)) / (x2 - x1));
}

inline float tx_envelope_process_sample(tx_envelope& e, bool gate, bool trigger, float _attack_mod = 0,
										float _decay_mod = 0)
{
	size_t attack_mid_x1 = tx_mtos(e.attack1_rate + (float)_attack_mod, e.samplerate);
	size_t attack_mid_x2 = tx_mtos(e.attack2_rate + (float)_attack_mod, e.samplerate);
	size_t hold_samp = tx_mtos(e.hold_rate, e.samplerate);
	size_t decay_mid_x1 = tx_mtos(e.decay1_rate + (float)_decay_mod, e.samplerate);
	size_t decay_mid_x2 = tx_mtos(e.decay2_rate + (float)_decay_mod, e.samplerate);
	size_t release_mid_x1 = tx_mtos(e.release1_rate + (float)_decay_mod, e.samplerate);
	size_t release_mid_x2 = tx_mtos(e.release2_rate + (float)_decay_mod, e.samplerate);

	// if note on is triggered, transition to attack phase
	if (trigger) {
		if (e.retrigger) e.start_level = 0.f;
		else e.start_level = e.level;
		e.phase = 0;
		e.state = attack1;
	}
	// attack 1st half
	if (e.state == attack1) {
		// while in attack phase
		if (e.phase < attack_mid_x1) {
			e.level = tx_lerp(0, e.start_level, (float)attack_mid_x1, e.attack1_level, (float)e.phase);
			e.phase += 1;
		}
		// reset phase if parameter was changed
		if (e.phase > attack_mid_x1) { e.phase = attack_mid_x1; }
		// if attack phase is done, transition to decay phase
		if (e.phase == attack_mid_x1) {
			e.state = attack2;
			e.phase = 0;
		}
	}
	// attack 2nd half
	if (e.state == attack2) {
		// while in attack phase
		if (e.phase < attack_mid_x2) {
			e.level = tx_lerp(0, e.attack1_level, (float)attack_mid_x2, 1, (float)e.phase);
			e.phase += 1;
		}
		// reset phase if parameter was changed
		if (e.phase > attack_mid_x2) { e.phase = attack_mid_x2; }
		// if attack phase is done, transition to decay phase
		if (e.phase == attack_mid_x2) {
			e.state = hold;
			e.phase = 0;
		}
	}
	// hold
	if (e.state == hold) {
		if (e.phase < hold_samp) {
			e.level = 1.0;
			e.phase += 1;
		}
		if (e.phase > hold_samp) { e.phase = hold_samp; }
		if (e.phase == hold_samp) {
			e.state = decay1;
			e.phase = 0;
		}
	}
	// decay 1st half
	if (e.state == decay1) {
		// while in decay phase
		if (e.phase < decay_mid_x1) {
			e.level = tx_lerp(0, 1, (float)decay_mid_x1, e.decay1_level, (float)e.phase);
			e.phase += 1;
		}
		// reset phase if parameter was changed
		if (e.phase > decay_mid_x1) { e.phase = decay_mid_x1; }
		// if decay phase is done, transition to sustain phase
		if (e.phase == decay_mid_x1) {
			e.state = decay2;
			e.phase = 0;
		}
	}
	// decay 2nd half
	if (e.state == decay2) {
		// while in decay phase
		if (e.phase < decay_mid_x2) {
			e.level = tx_lerp(0, e.decay1_level, (float)decay_mid_x2, e.sustain_level, (float)e.phase);
			e.phase += 1;
		}
		// reset phase if parameter was changed
		if (e.phase > decay_mid_x2) { e.phase = decay_mid_x2; }
		// if decay phase is done, transition to sustain phase
		if (e.phase == decay_mid_x2) {
			e.state = sustain;
			e.phase = 0;
			e.level = e.sustain_level;
		}
	}
	// while sustain phase: if note off is triggered, transition to release phase
	if (e.state == sustain && (!gate || e.skip_sustain)) {
		e.state = release1;
		e.level = e.sustain_level;
	}
	// release 1st half
	if (e.state == release1) {
		// while in release phase
		if (e.phase < release_mid_x1) {
			e.level = tx_lerp(0, e.sustain_level, (float)release_mid_x1, e.release1_level, (float)e.phase);
			e.phase += 1;
		}
		// reset phase if parameter was changed
		if (e.phase > release_mid_x1) { e.phase = release_mid_x1; }
		// transition to 2nd release half
		if (e.phase == release_mid_x1) {
			e.phase = 0;
			e.state = release2;
		}
	}
	// release 2nd half
	if (e.state == release2) {
		// while in release phase
		if (e.phase < release_mid_x2) {
			e.level = tx_lerp(0, e.release1_level, (float)release_mid_x2, 0, (float)e.phase);
			e.phase += 1;
		}
		// reset phase if parameter was changed
		if (e.phase > release_mid_x2) { e.phase = release_mid_x2; }
		// reset
		if (e.phase == release_mid_x2) {
			e.phase = 0;
			e.state = idle;
			e.level = 0;
		}
	}

	// smooth output
	e.h3 = e.h2;
	e.h2 = e.h1;
	e.h1 = e.level;

	return (e.h1 + e.h2 + e.h3) / 3.f;
}

//////////////
// OPERATOR //
//////////////

struct tx_operator {
	tx_envelope envelope;
	tx_sineosc oscillator;
	float ratio = 1.f;
	float amplitude = 1.f;
};

inline void tx_operator_init(tx_operator& op, double samplerate)
{
	tx_envelope_init(op.envelope, samplerate);
	tx_sineosc_init(op.oscillator, samplerate);
}

inline float tx_operator_process_sample(tx_operator& op, bool gate, bool trigger, float frequency, float velocity,
										float pm = 0.f)
{
	float env = tx_envelope_process_sample(op.envelope, gate, trigger);

	// drifts and sounds better!
	if (op.envelope.state != idle) {
		float osc = tx_sineosc_process_sample(op.oscillator, trigger, frequency, pm);
		return osc * env * velocity;
	} else {
		return 0.f;
	}
}

////////////
// VOICE  //
////////////

constexpr float MOD_INDEX_COEFF = 4.f;

struct tx_state {
	float additional_pitch_mod = 0.f; // modulates pitch in frequency
	int algorithm = 0;
	float pitch_env_amt = 0.f;
	float pitch_mod = 0.f;
	float feedback_amt = 0.f;
	float bit_resolution = 12.f;
	tx_sineosc feedback_osc;
	tx_envelope pitch_env;
	tx_operator op1;
	tx_operator op2;
	tx_operator op3;
};

inline void tx_voice_process_block(tx_state& t, voice_state& s, float** audio, size_t num_frames,
								   const vector<audio_buffer<float>>& mods)
{
	float frequency = midi_to_frequency(s.midi_note + t.pitch_mod + t.additional_pitch_mod);

	for (int i = 0; i < num_frames; i++) {

		voice_process_event_for_frame(s, i);

		float pitch_env_signal = tx_envelope_process_sample(t.pitch_env, s.gate, s.trigger) * t.pitch_env_amt;
		float pitched_freq = frequency + pitch_env_signal;

		float output = 0.f;

		// mix operator signals according to selected algorithm
		if (t.algorithm == 0) {
			float fb_freq = frequency * t.op3.ratio;
			float fb_mod_index = (t.feedback_amt * MOD_INDEX_COEFF);
			float fb_signal = tx_sineosc_process_sample(t.feedback_osc, s.trigger, fb_freq) * fb_mod_index;

			float op3_Freq = frequency * t.op3.ratio;
			float op3_mod_index = (t.op3.amplitude * MOD_INDEX_COEFF);
			float op3_signal =
				tx_operator_process_sample(t.op3, s.gate, s.trigger, op3_Freq, s.velocity, fb_signal) * op3_mod_index;

			float op2_freq = frequency * t.op2.ratio;
			float op2_mod_index = (t.op2.amplitude * MOD_INDEX_COEFF);
			float op2_signal =
				tx_operator_process_sample(t.op2, s.gate, s.trigger, op2_freq, s.velocity, op3_signal) * op2_mod_index;

			float op1_freq = frequency * t.op1.ratio;
			output = tx_operator_process_sample(t.op1, s.gate, s.trigger, op1_freq, s.velocity, op2_signal) *
					 t.op1.amplitude;
		} else if (t.algorithm == 1) {
			float fb_freq = frequency * t.op3.ratio;
			float fb_mod_index = (t.feedback_amt * MOD_INDEX_COEFF);
			float fb_signal = tx_sineosc_process_sample(t.feedback_osc, s.trigger, fb_freq) * fb_mod_index;

			float op3_freq = frequency * t.op3.ratio;
			float op3_signal =
				tx_operator_process_sample(t.op3, s.gate, s.trigger, op3_freq, s.velocity, fb_signal) * t.op3.amplitude;

			float op2_freq = frequency * t.op2.ratio;
			float op2_mod_index = (t.op2.amplitude * MOD_INDEX_COEFF);
			float op2_signal =
				tx_operator_process_sample(t.op2, s.gate, s.trigger, op2_freq, s.velocity) * op2_mod_index;

			float op1_freq = frequency * t.op1.ratio;
			float op1_signal = tx_operator_process_sample(t.op1, s.gate, s.trigger, op1_freq, s.velocity, op2_signal) *
							   t.op1.amplitude;

			output = op1_signal + op3_signal;
		} else if (t.algorithm == 2) {
			float fb_freq = frequency * t.op3.ratio;
			float fb_mod_index = (t.feedback_amt * MOD_INDEX_COEFF);
			float fb_signal = tx_sineosc_process_sample(t.feedback_osc, s.trigger, fb_freq) * fb_mod_index;

			float op3_freq = frequency * t.op3.ratio;
			float op3_signal =
				tx_operator_process_sample(t.op3, s.gate, s.trigger, op3_freq, s.velocity, fb_signal) * t.op3.amplitude;

			float op2_freq = frequency * t.op2.ratio;
			float op2_signal =
				tx_operator_process_sample(t.op2, s.gate, s.trigger, op2_freq, s.velocity) * t.op2.amplitude;

			float op1_freq = frequency * t.op1.ratio;
			float op1_signal =
				tx_operator_process_sample(t.op1, s.gate, s.trigger, op1_freq, s.velocity) * t.op1.amplitude;

			output = op1_signal + op2_signal + op3_signal;
		} else if (t.algorithm == 3) {
			float fb_freq = frequency * t.op3.ratio;
			float fb_mod_index = (t.feedback_amt * MOD_INDEX_COEFF);
			float fb_signal = tx_sineosc_process_sample(t.feedback_osc, s.trigger, fb_freq) * fb_mod_index;

			float op3_freq = frequency * t.op3.ratio;
			float op3_mod_index = (t.op3.amplitude * MOD_INDEX_COEFF);
			float op3_signal =
				tx_operator_process_sample(t.op3, s.gate, s.trigger, op3_freq, s.velocity, fb_signal) * op3_mod_index;

			float op2_freq = frequency * t.op2.ratio;
			float op2_mod_index = (t.op2.amplitude * MOD_INDEX_COEFF);
			float op2_signal =
				tx_operator_process_sample(t.op2, s.gate, s.trigger, op2_freq, s.velocity) * op2_mod_index;

			float op1_freq = frequency * t.op1.ratio;
			output =
				tx_operator_process_sample(t.op1, s.gate, s.trigger, op1_freq, s.velocity, op2_signal + op3_signal) *
				t.op1.amplitude;
		}

		// reset trigger
		s.trigger = false;

		float res = powf(2, t.bit_resolution);
		output = roundf(output * res) / res;

		audio[0][i] += output / 3.;
		audio[1][i] = audio[0][i];
	}
}

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

	float apply(float _input) const
	{
		if (range_min == range_max && exponent == 1.f) return _input;

		return powf(_input, exponent) * (range_max - range_min) + range_min;
	}
};

struct tx_synth {
	voice_allocator allocator;
	array<tx_state, MAX_VOICES> voices;
};

inline void tx_synth_process_block(tx_synth& s, float** audio, size_t num_frames, const vector<midi_event>& midi_events,
								   const vector<audio_buffer<float>>& mods)
{
	voice_allocator_process_block(s.allocator, midi_events);

	for (int i = 0; i < MAX_VOICES; i++) {
		tx_voice_process_block(s.voices[i], s.allocator.voices[i], audio, num_frames, mods);
	}
}

inline void tx_apply_parameter_mapping(array<tx_state, MAX_VOICES>& v, tx_parameter_mapping& m, float value)
{
	if (m.range_min != m.range_max || m.exponent != 1.f)
		value = powf(value, m.exponent) * (m.range_max - m.range_min) + m.range_min;

	for (int i = 0; i < MAX_VOICES; i++) {
		tx_state& s = v[i];

		switch (m.parameter) {
		case tx_parameter::BIT_RESOLUTION:
			s.bit_resolution = value;
			break;
		case tx_parameter::FEEDBACKOSC_PHASE_RESOLUTION:
			s.feedback_osc.phase_resolution = value;
			break;
		case tx_parameter::FEEDBACK:
			s.feedback_amt = value;
			break;
		case tx_parameter::ALGORITHM:
			s.algorithm = value;
			break;
		case tx_parameter::PITCH_ENVELOPE_AMOUNT:
			s.pitch_env_amt = value;
			break;
		case tx_parameter::PITCH_ENVELOPE_SKIP_SUSTAIN:
			s.pitch_env.skip_sustain = value;
			break;
		case tx_parameter::PITCH_ENVELOPE_ATTACK1_RATE:
			s.pitch_env.attack1_rate = value;
			break;
		case tx_parameter::PITCH_ENVELOPE_ATTACK1_LEVEL:
			s.pitch_env.attack1_level = value;
			break;
		case tx_parameter::PITCH_ENVELOPE_ATTACK2_RATE:
			s.pitch_env.attack2_rate = value;
			break;
		case tx_parameter::PITCH_ENVELOPE_HOLD_RATE:
			s.pitch_env.hold_rate = value;
			break;
		case tx_parameter::PITCH_ENVELOPE_DECAY1_RATE:
			s.pitch_env.decay1_rate = value;
			break;
		case tx_parameter::PITCH_ENVELOPE_DECAY1_LEVEL:
			s.pitch_env.decay1_level = value;
			break;
		case tx_parameter::PITCH_ENVELOPE_DECAY2_RATE:
			s.pitch_env.decay2_rate = value;
			break;
		case tx_parameter::PITCH_ENVELOPE_SUSTAIN_LEVEL:
			s.pitch_env.sustain_level = value;
			break;
		case tx_parameter::PITCH_ENVELOPE_RELEASE1_RATE:
			s.pitch_env.release1_rate = value;
			break;
		case tx_parameter::PITCH_ENVELOPE_RELEASE1_LEVEL:
			s.pitch_env.release1_level = value;
			break;
		case tx_parameter::PITCH_ENVELOPE_RELEASE2_RATE:
			s.pitch_env.release2_rate = value;
			break;
		case tx_parameter::OP1_RATIO:
			s.op1.ratio = value;
			break;
		case tx_parameter::OP1_AMPLITUDE:
			s.op1.amplitude = value;
			break;
		case tx_parameter::OP1_PHASE_RESOLUTION:
			s.op1.oscillator.phase_resolution = value;
			break;
		case tx_parameter::OP1_ENVELOPE_SKIP_SUSTAIN:
			s.op1.envelope.skip_sustain = value;
			break;
		case tx_parameter::OP1_ENVELOPE_ATTACK1_RATE:
			s.op1.envelope.attack1_rate = value;
			break;
		case tx_parameter::OP1_ENVELOPE_ATTACK1_LEVEL:
			s.op1.envelope.attack1_level = value;
			break;
		case tx_parameter::OP1_ENVELOPE_ATTACK2_RATE:
			s.op1.envelope.attack2_rate = value;
			break;
		case tx_parameter::OP1_ENVELOPE_HOLD_RATE:
			s.op1.envelope.hold_rate = value;
			break;
		case tx_parameter::OP1_ENVELOPE_DECAY1_RATE:
			s.op1.envelope.decay1_rate = value;
			break;
		case tx_parameter::OP1_ENVELOPE_DECAY1_LEVEL:
			s.op1.envelope.decay1_level = value;
			break;
		case tx_parameter::OP1_ENVELOPE_DECAY2_RATE:
			s.op1.envelope.decay2_rate = value;
			break;
		case tx_parameter::OP1_ENVELOPE_SUSTAIN_LEVEL:
			s.op1.envelope.sustain_level = value;
			break;
		case tx_parameter::OP1_ENVELOPE_RELEASE1_RATE:
			s.op1.envelope.release1_rate = value;
			break;
		case tx_parameter::OP1_ENVELOPE_RELEASE1_LEVEL:
			s.op1.envelope.release1_level = value;
			break;
		case tx_parameter::OP1_ENVELOPE_RELEASE2_RATE:
			s.op1.envelope.release2_rate = value;
			break;
		case tx_parameter::OP2_RATIO:
			s.op2.ratio = value;
			break;
		case tx_parameter::OP2_AMPLITUDE:
			s.op2.amplitude = value;
			break;
		case tx_parameter::OP2_PHASE_RESOLUTION:
			s.op2.oscillator.phase_resolution = value;
			break;
		case tx_parameter::OP2_ENVELOPE_SKIP_SUSTAIN:
			s.op2.envelope.skip_sustain = value;
			break;
		case tx_parameter::OP2_ENVELOPE_ATTACK1_RATE:
			s.op2.envelope.attack1_rate = value;
			break;
		case tx_parameter::OP2_ENVELOPE_ATTACK1_LEVEL:
			s.op2.envelope.attack1_level = value;
			break;
		case tx_parameter::OP2_ENVELOPE_ATTACK2_RATE:
			s.op2.envelope.attack2_rate = value;
			break;
		case tx_parameter::OP2_ENVELOPE_HOLD_RATE:
			s.op2.envelope.hold_rate = value;
			break;
		case tx_parameter::OP2_ENVELOPE_DECAY1_RATE:
			s.op2.envelope.decay1_rate = value;
			break;
		case tx_parameter::OP2_ENVELOPE_DECAY1_LEVEL:
			s.op2.envelope.decay1_level = value;
			break;
		case tx_parameter::OP2_ENVELOPE_DECAY2_RATE:
			s.op2.envelope.decay2_rate = value;
			break;
		case tx_parameter::OP2_ENVELOPE_SUSTAIN_LEVEL:
			s.op2.envelope.sustain_level = value;
			break;
		case tx_parameter::OP2_ENVELOPE_RELEASE1_RATE:
			s.op2.envelope.release1_rate = value;
			break;
		case tx_parameter::OP2_ENVELOPE_RELEASE1_LEVEL:
			s.op2.envelope.release1_level = value;
			break;
		case tx_parameter::OP2_ENVELOPE_RELEASE2_RATE:
			s.op2.envelope.release2_rate = value;
			break;
		case tx_parameter::OP3_RATIO:
			s.op3.ratio = value;
			break;
		case tx_parameter::OP3_AMPLITUDE:
			s.op3.amplitude = value;
			break;
		case tx_parameter::OP3_PHASE_RESOLUTION:
			s.op3.oscillator.phase_resolution = value;
			break;
		case tx_parameter::OP3_ENVELOPE_SKIP_SUSTAIN:
			s.op3.envelope.skip_sustain = value;
			break;
		case tx_parameter::OP3_ENVELOPE_ATTACK1_RATE:
			s.op3.envelope.attack1_rate = value;
			break;
		case tx_parameter::OP3_ENVELOPE_ATTACK1_LEVEL:
			s.op3.envelope.attack1_level = value;
			break;
		case tx_parameter::OP3_ENVELOPE_ATTACK2_RATE:
			s.op3.envelope.attack2_rate = value;
			break;
		case tx_parameter::OP3_ENVELOPE_HOLD_RATE:
			s.op3.envelope.hold_rate = value;
			break;
		case tx_parameter::OP3_ENVELOPE_DECAY1_RATE:
			s.op3.envelope.decay1_rate = value;
			break;
		case tx_parameter::OP3_ENVELOPE_DECAY1_LEVEL:
			s.op3.envelope.decay1_level = value;
			break;
		case tx_parameter::OP3_ENVELOPE_DECAY2_RATE:
			s.op3.envelope.decay2_rate = value;
			break;
		case tx_parameter::OP3_ENVELOPE_SUSTAIN_LEVEL:
			s.op3.envelope.sustain_level = value;
			break;
		case tx_parameter::OP3_ENVELOPE_RELEASE1_RATE:
			s.op3.envelope.release1_rate = value;
			break;
		case tx_parameter::OP3_ENVELOPE_RELEASE1_LEVEL:
			s.op3.envelope.release1_level = value;
			break;
		case tx_parameter::OP3_ENVELOPE_RELEASE2_RATE:
			s.op3.envelope.release2_rate = value;
			break;
		}
	}
}

} // namespace trnr

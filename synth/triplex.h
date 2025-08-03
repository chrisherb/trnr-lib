#pragma once
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

inline float tx_enveloope_process_sample(tx_envelope& e, bool gate, bool trigger, float _attack_mod, float _decay_mod)
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
	float ratio;
	float amplitude;
};

} // namespace trnr

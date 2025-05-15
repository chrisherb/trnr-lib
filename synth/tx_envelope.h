#pragma once
#include <array>
#include <cmath>

namespace trnr {

enum env_state {
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

class tx_envelope {
public:
	env_state state = idle;
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

	tx_envelope(bool _retrigger = false)
		: retrigger {_retrigger}
	{
	}

	float process_sample(bool gate, bool trigger) { return process_sample<float>(gate, trigger, 0, 0); }

	template <typename t_sample>
	float process_sample(bool gate, bool trigger, t_sample _attack_mod, t_sample _decay_mod)
	{
		size_t attack_mid_x1 = ms_to_samples(attack1_rate + (float)_attack_mod);
		size_t attack_mid_x2 = ms_to_samples(attack2_rate + (float)_attack_mod);
		size_t hold_samp = ms_to_samples(hold_rate);
		size_t decay_mid_x1 = ms_to_samples(decay1_rate + (float)_decay_mod);
		size_t decay_mid_x2 = ms_to_samples(decay2_rate + (float)_decay_mod);
		size_t release_mid_x1 = ms_to_samples(release1_rate + (float)_decay_mod);
		size_t release_mid_x2 = ms_to_samples(release2_rate + (float)_decay_mod);

		// if note on is triggered, transition to attack phase
		if (trigger) {
			if (retrigger) start_level = 0.f;
			else start_level = level;
			phase = 0;
			state = attack1;
		}
		// attack 1st half
		if (state == attack1) {
			// while in attack phase
			if (phase < attack_mid_x1) {
				level = lerp(0, start_level, (float)attack_mid_x1, attack1_level, (float)phase);
				phase += 1;
			}
			// reset phase if parameter was changed
			if (phase > attack_mid_x1) { phase = attack_mid_x1; }
			// if attack phase is done, transition to decay phase
			if (phase == attack_mid_x1) {
				state = attack2;
				phase = 0;
			}
		}
		// attack 2nd half
		if (state == attack2) {
			// while in attack phase
			if (phase < attack_mid_x2) {
				level = lerp(0, attack1_level, (float)attack_mid_x2, 1, (float)phase);
				phase += 1;
			}
			// reset phase if parameter was changed
			if (phase > attack_mid_x2) { phase = attack_mid_x2; }
			// if attack phase is done, transition to decay phase
			if (phase == attack_mid_x2) {
				state = hold;
				phase = 0;
			}
		}
		// hold
		if (state == hold) {
			if (phase < hold_samp) {
				level = 1.0;
				phase += 1;
			}
			if (phase > hold_samp) { phase = hold_samp; }
			if (phase == hold_samp) {
				state = decay1;
				phase = 0;
			}
		}
		// decay 1st half
		if (state == decay1) {
			// while in decay phase
			if (phase < decay_mid_x1) {
				level = lerp(0, 1, (float)decay_mid_x1, decay1_level, (float)phase);
				phase += 1;
			}
			// reset phase if parameter was changed
			if (phase > decay_mid_x1) { phase = decay_mid_x1; }
			// if decay phase is done, transition to sustain phase
			if (phase == decay_mid_x1) {
				state = decay2;
				phase = 0;
			}
		}
		// decay 2nd half
		if (state == decay2) {
			// while in decay phase
			if (phase < decay_mid_x2) {
				level = lerp(0, decay1_level, (float)decay_mid_x2, sustain_level, (float)phase);
				phase += 1;
			}
			// reset phase if parameter was changed
			if (phase > decay_mid_x2) { phase = decay_mid_x2; }
			// if decay phase is done, transition to sustain phase
			if (phase == decay_mid_x2) {
				state = sustain;
				phase = 0;
				level = sustain_level;
			}
		}
		// while sustain phase: if note off is triggered, transition to release phase
		if (state == sustain && (!gate || skip_sustain)) {
			state = release1;
			level = sustain_level;
		}
		// release 1st half
		if (state == release1) {
			// while in release phase
			if (phase < release_mid_x1) {
				level = lerp(0, sustain_level, (float)release_mid_x1, release1_level, (float)phase);
				phase += 1;
			}
			// reset phase if parameter was changed
			if (phase > release_mid_x1) { phase = release_mid_x1; }
			// transition to 2nd release half
			if (phase == release_mid_x1) {
				phase = 0;
				state = release2;
			}
		}
		// release 2nd half
		if (state == release2) {
			// while in release phase
			if (phase < release_mid_x2) {
				level = lerp(0, release1_level, (float)release_mid_x2, 0, (float)phase);
				phase += 1;
			}
			// reset phase if parameter was changed
			if (phase > release_mid_x2) { phase = release_mid_x2; }
			// reset
			if (phase == release_mid_x2) {
				phase = 0;
				state = idle;
				level = 0;
			}
		}

		return smooth(level);
	}

	bool is_busy() { return state != 0; }

	void set_samplerate(double sampleRate) { this->samplerate = sampleRate; }

	// converts the x/y coordinates of the envelope points as a list for graphical representation.
	std::array<float, 18> calc_coordinates(float _max_attack, float _max_decay, float _max_release)
	{

		auto scale = [](float _value, float _max) { return powf(_value / _max, 0.25) * _max; };

		float a_x = 0;
		float a_y = 0;

		float b_x = scale(attack1_rate, _max_attack / 2);
		float b_y = attack1_level;

		float c_x = b_x + scale(attack2_rate, _max_attack / 2);
		float c_y = 1;

		float d_x = c_x + hold_rate;
		float d_y = 1;

		float e_x = d_x + scale(decay1_rate, _max_decay / 2);
		float e_y = decay1_level;

		float f_x = e_x + scale(decay2_rate, _max_decay / 2);
		float f_y = sustain_level;

		float g_x = _max_attack + _max_decay;
		float g_y = sustain_level;

		float h_x = g_x + scale(release1_rate, _max_decay / 2);
		float h_y = release1_level;

		float i_x = h_x + scale(release2_rate, _max_decay / 2);
		float i_y = 0;

		float total = _max_attack + _max_decay + _max_release;

		return {a_x,		 a_y, b_x / total, b_y, c_x / total, c_y, d_x / total, d_y, e_x / total, e_y,
				f_x / total, f_y, g_x / total, g_y, h_x / total, h_y, i_x / total, i_y};
	}

private:
	double samplerate = 44100.;
	size_t phase = 0;
	float level = 0.f;
	float start_level = 0.f;
	float h1 = 0.f;
	float h2 = 0.f;
	float h3 = 0.f;
	bool retrigger;

	float lerp(float x1, float y1, float x2, float y2, float x) { return y1 + (((x - x1) * (y2 - y1)) / (x2 - x1)); }

	float smooth(float sample)
	{
		h3 = h2;
		h2 = h1;
		h1 = sample;

		return (h1 + h2 + h3) / 3.f;
	}

	size_t ms_to_samples(float ms) { return static_cast<size_t>(ms * samplerate / 1000.f); }
};
} // namespace trnr
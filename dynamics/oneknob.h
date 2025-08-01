#pragma once
#include "audio_math.h"
#include "rms_detector.h"
#include <algorithm>
#include <cmath>

namespace trnr {
struct hp_filter {
	float a0, a1, b1;
	float z1; // filter state
};

inline void hp_filter_init(hp_filter& f, float samplerate)
{
	const float cutoff = 100.0f;
	float w0 = 2.0f * 3.14159265359f * cutoff / samplerate;
	float alpha = (1.0f - std::tan(w0 / 2.0f)) / (1.0f + std::tan(w0 / 2.0f));
	f.a0 = 0.5f * (1.0f + alpha);
	f.a1 = -0.5f * (1.0f + alpha);
	f.b1 = alpha;
	f.z1 = 0.0f;
}

inline float hp_filter_process(hp_filter& f, float x)
{
	float y = f.a0 * x + f.a1 * f.z1 - f.b1 * f.z1;
	f.z1 = x;
	return y;
}

struct oneknob_comp {
	// params
	float amount = 0.f;
	bool multiplied = false;

	// state
	rms_detector detector;
	hp_filter filter;
	float attack_coef;
	float release_coef;
	float envelope_level;
	float sidechain_in;
	double samplerate;
};

inline void oneknob_init(oneknob_comp& c, double samplerate, float window_ms)
{
	rms_init(c.detector, samplerate, window_ms);
	hp_filter_init(c.filter, samplerate);

	const float attack_ms = 0.2f;
	const float release_ms = 150.f;

	c.samplerate = samplerate;
	c.attack_coef = expf(-1.0f / (attack_ms * 1e-6 * samplerate));
	c.release_coef = expf(-1.0f / (release_ms * 1e-3 * samplerate));
	c.envelope_level = -60.f;
	c.sidechain_in = 0.f;
}

template <typename sample>
inline void oneknob_process_block(oneknob_comp& c, sample** audio, int frames)
{
	const float min_user_ratio = 1.0f;
	const float max_user_ratio = 20.0f;
	const float threshold_db = -12.f;

	const float amount = fmaxf(0.0f, fminf(powf(c.amount, 2.f), 1.0f));
	float ratio = min_user_ratio + amount * (max_user_ratio - min_user_ratio);

	const float fast_attack = 0.1f;
	const float slow_attack = 0.6f;
	const float fast_release = 90.f;
	const float slow_release = 300.f;
	const float max_gr = 12.f;

	for (int i = 0; i < frames; ++i) {
		float rms_value = rms_process<sample>(c.detector, c.sidechain_in);
		float envelope_in = lin_2_db(fmaxf(fabs(rms_value), 1e-20f));

		// attack
		if (envelope_in > c.envelope_level) {
			c.envelope_level = envelope_in + c.attack_coef * (c.envelope_level - envelope_in);
		}
		// release
		else {
			c.envelope_level = envelope_in + c.release_coef * (c.envelope_level - envelope_in);
		}

		float x = c.envelope_level;
		float y;

		if (x < threshold_db) y = x;
		else y = threshold_db + (x - threshold_db) / ratio;

		float gain_reduction_db = y - x;
		float gain_reduction_lin = db_2_lin(gain_reduction_db);

		audio[0][i] *= gain_reduction_lin;
		audio[1][i] *= gain_reduction_lin;

		// calculate program-dependent attack/release times
		float norm_gr = std::clamp(gain_reduction_db / max_gr, 0.f, 1.f);
		float release_ms = fast_release + (slow_release - fast_release) * (1.f - norm_gr);
		float attack_ms = fast_attack + (slow_attack - fast_attack) * (1.f - norm_gr);

		c.attack_coef = expf(-1.0f / (attack_ms * 1e-6 * c.samplerate));
		c.release_coef = expf(-1.0f / (release_ms * 1e-3 * c.samplerate));

		// feedback compression
		float sum = sqrtf(0.5f * (audio[0][i] * audio[0][i] + audio[1][i] * audio[1][i]));
		if (c.multiplied) sum *= 3.f;
		c.sidechain_in = hp_filter_process(c.filter, sum);
	}
}
} // namespace trnr

#pragma once
#include "audio_math.h"
#include "rms_detector.h"
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
	float amount;

	// state
	rms_detector detector;
	hp_filter filter;
	float attack_coef;
	float release_coef;
	float envelope_level;
	float sidechain_in;
};

inline void oneknob_init(oneknob_comp& c, float samplerate, float window_ms)
{
	rms_init(c.detector, samplerate, window_ms);
	hp_filter_init(c.filter, samplerate);

	const float attack_ms = 0.2f;
	const float release_ms = 150.f;

	// c.amount = 0.f;
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
	const float threshold_db = -9.f;

	const float amount = fmaxf(0.0f, fminf(powf(c.amount, 2.f), 1.0f)); // clamp to [0, 1]
	float ratio = min_user_ratio + amount * (max_user_ratio - min_user_ratio);

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

		// feedback compression
		float sum = sqrtf(0.5f * (audio[0][i] * audio[0][i] + audio[1][i] * audio[1][i]));
		c.sidechain_in = hp_filter_process(c.filter, sum);
	}
}
} // namespace trnr

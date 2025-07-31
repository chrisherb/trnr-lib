#pragma once
#include "audio_math.h"
#include <cmath>

namespace trnr {

struct rms_detector {
	float alpha;
	float rms_squared;
};

inline void rms_init(rms_detector& det, float samplerate, float window_ms)
{
	float window_seconds = 0.001f * window_ms;
	det.alpha = 1.0f - expf(-1.0f / (samplerate * window_seconds));
	det.rms_squared = 0.0f;
}

template <typename sample>
inline sample rms_process(rms_detector& det, sample input)
{
	det.rms_squared = (1.0f - det.alpha) * det.rms_squared + det.alpha * (input * input);
	return sqrtf(det.rms_squared);
}

struct hp_filter {
	float a0, a1, b1;
	float z1; // filter state
};

inline void hp_filter_init(hp_filter& f, float samplerate)
{
	float cutoff = 100.0f;
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

inline void oneknob_init(oneknob_comp& comp, float samplerate, float window_ms)
{
	rms_init(comp.detector, samplerate, window_ms);
	hp_filter_init(comp.filter, samplerate);

	const float attack_ms = 0.2f;
	const float release_ms = 150.f;

	comp.attack_coef = expf(-1.0f / (attack_ms * 1e-6 * samplerate));
	comp.release_coef = expf(-1.0f / (release_ms * 1e-3 * samplerate));
	comp.envelope_level = -60.f;
	comp.sidechain_in = 0.f;
}

template <typename sample>
inline void oneknob_process_block(oneknob_comp& comp, sample** audio, int frames)
{
	const float min_user_ratio = 1.0f;
	const float max_user_ratio = 20.0f;
	const float threshold_db = -9.f;

	const float amount = fmaxf(0.0f, fminf(powf(comp.amount, 2.f), 1.0f)); // clamp to [0, 1]
	float ratio = min_user_ratio + amount * (max_user_ratio - min_user_ratio);

	for (int i = 0; i < frames; ++i) {
		float rms_value = rms_process(comp.detector, comp.sidechain_in);
		float envelope_in = lin_2_db(fmaxf(fabs(rms_value), 1e-20f));

		// attack
		if (envelope_in > comp.envelope_level) {
			comp.envelope_level = envelope_in + comp.attack_coef * (comp.envelope_level - envelope_in);
		}
		// release
		else {
			comp.envelope_level = envelope_in + comp.release_coef * (comp.envelope_level - envelope_in);
		}

		float x = comp.envelope_level;
		float y;

		if (x < threshold_db) y = x;
		else y = threshold_db + (x - threshold_db) / ratio;

		float gain_reduction_db = y - x;
		float gain_reduction_lin = db_2_lin(gain_reduction_db);

		audio[0][i] *= gain_reduction_lin;
		audio[1][i] *= gain_reduction_lin;

		// feedback compression
		float sum = sqrtf(0.5f * (audio[0][i] * audio[0][i] + audio[1][i] * audio[1][i]));
		comp.sidechain_in = hp_filter_process(comp.filter, sum);
	}
}

} // namespace trnr

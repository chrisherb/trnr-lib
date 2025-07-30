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
	comp.amount = 0.0f;

	const float attack_ms = 10.f;
	const float release_ms = 100.f;

	comp.attack_coef = expf(-1.0f / (samplerate * (attack_ms * 0.001f)));
	comp.release_coef = expf(-1.0f / (samplerate * (release_ms * 0.001f)));
	comp.envelope_level = 0.f;
	comp.sidechain_in = 0.f;
}

template <typename sample>
inline void oneknob_process_block(oneknob_comp& comp, sample** audio, int frames)
{
	const float threshold = -18.f;
	const float min_ratio = 1.0f;
	const float max_ratio = 10.0f;
	float ratio = min_ratio + comp.amount * (max_ratio - min_ratio);

	for (int i = 0; i < frames; ++i) {
		float rms_value = rms_process(comp.detector, comp.sidechain_in);
		float absolute_rms_db = lin_2_db(fabs(rms_value));

		// cut envelope below threshold
		float overshoot = absolute_rms_db - threshold;
		if (overshoot < 0.f) overshoot = 0.f;

		if (overshoot > comp.envelope_level) {
			comp.envelope_level = overshoot + comp.attack_coef * (comp.envelope_level - overshoot);
		} else {
			comp.envelope_level = overshoot + comp.release_coef * (comp.envelope_level - overshoot);
		}

		if (comp.envelope_level < 0.f) comp.envelope_level = 0.f;

		float slope = 1.f / ratio;

		float gain_reduction_db = comp.envelope_level * (slope - 1.f);
		float gain_reduction_lin = db_2_lin(gain_reduction_db);

		sample input_l = audio[0][i];
		sample input_r = audio[1][i];

		audio[0][i] *= gain_reduction_lin;
		audio[1][i] *= gain_reduction_lin;

		// feedback compression
		float sum = sqrtf(0.5f * (audio[0][i] * audio[0][i] + audio[1][i] * audio[1][i]));
		comp.sidechain_in = hp_filter_process(comp.filter, sum);
	}
}

} // namespace trnr

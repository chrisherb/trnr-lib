#pragma once
#include <cmath>

namespace trnr {
struct rms_detector {
	float alpha;
	float rms_squared;
};

inline void rms_init(rms_detector& d, float samplerate, float window_ms)
{
	float window_seconds = 0.001f * window_ms;
	d.alpha = 1.0f - expf(-1.0f / (samplerate * window_seconds));
	d.rms_squared = 0.0f;
}

template <typename sample>
inline sample rms_process(rms_detector& d, sample input)
{
	d.rms_squared = (1.0f - d.alpha) * d.rms_squared + d.alpha * (input * input);
	return sqrtf(d.rms_squared);
}
} // namespace trnr

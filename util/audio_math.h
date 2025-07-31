#pragma once

#include <math.h>

namespace trnr {
inline double lin_2_db(double lin)
{
	if (lin <= 1e-20) lin = 1e-20; // avoid log(0)
	return 20.0 * log10(lin);
}

inline double db_2_lin(double db) { return pow(10.0, db / 20.0); }

inline float midi_to_frequency(float midi_note) { return 440.0 * powf(2.0, ((float)midi_note - 69.0) / 12.0); }
} // namespace trnr
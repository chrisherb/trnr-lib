#pragma once

#include <math.h>

namespace trnr {
	static inline double lin_2_db(double lin) {
		return 20 * log(lin);
	}

	static inline double db_2_lin(double db) {
		return pow(10, db/20);
	}

	static inline float midi_to_frequency(float midi_note) {
		return 440.0 * powf(2.0, ((float)midi_note - 69.0) / 12.0);
	}
}
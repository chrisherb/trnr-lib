#pragma once

#include <math.h>

double trnr_lin_2_db(double lin) { return 20 * log(lin); }

double trnr_db_2_lin(double db) { return pow(10, db / 20); }

float trnr_midi_to_frequency(float midi_note) { return 440.0 * powf(2.0, ((float)midi_note - 69.0) / 12.0); }
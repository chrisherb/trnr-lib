/*
 * pump.h
 * Copyright (c) 2025 Christopher Herb
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "../util/audio_math.h"
#include <cmath>

namespace trnr {
struct pump {
	double samplerate;
	float threshold_db = 0.f;
	float attack_ms = 0.1f;
	float release_ms = 200.f;
	float hp_filter = 80.f;
	float ratio = 1000.f;
	float filter_frq = 20000.f;
	float filter_exp = 1.f;
	float treble_boost = 0.f;
    float makeup = 0.f;

	float filtered = 0.f;
	float filtered_l = 0.f;
	float filtered_r = 0.f;
	float boosted_l = 0.f;
	float boosted_r = 0.f;
	float attack_coef = 0.f;
	float release_coef = 0.f;
	float envelope_db = 0.f;
};

enum pump_param {
	PUMP_THRESHOLD,
	PUMP_ATTACK,
	PUMP_RELEASE,
	PUMP_HP_FILTER,
	PUMP_RATIO,
	PUMP_FILTER_FRQ,
	PUMP_FILTER_EXP,
	PUMP_TREBLE_BOOST
};

inline void pump_set_param(pump& p, pump_param param, float value)
{
	switch (param) {
	case PUMP_THRESHOLD:
		p.threshold_db = value;
		break;
	case PUMP_ATTACK:
		p.attack_ms = value;
		p.attack_coef = exp(-1000.0 / (p.attack_ms * p.samplerate));
		break;
	case PUMP_RELEASE:
		p.release_ms = value;
		p.release_coef = exp(-1000.0 / (p.release_ms * p.samplerate));
		break;
	case PUMP_HP_FILTER:
		p.hp_filter = value;
		break;
	case PUMP_RATIO:
		p.ratio = value;
		break;
	case PUMP_FILTER_FRQ:
		p.filter_frq = value;
		break;
	case PUMP_FILTER_EXP:
		p.filter_exp = value;
		break;
	case PUMP_TREBLE_BOOST:
		p.treble_boost = value;
		break;
	default:
		break;
	}
}

inline float pump_get_param(const pump& p, pump_param param)
{
	switch (param) {
	case PUMP_THRESHOLD:
		return p.threshold_db;
	case PUMP_ATTACK:
		return p.attack_ms;
	case PUMP_RELEASE:
		return p.release_ms;
	case PUMP_HP_FILTER:
		return p.hp_filter;
	case PUMP_RATIO:
		return p.ratio;
	case PUMP_FILTER_FRQ:
		return p.filter_frq;
	case PUMP_FILTER_EXP:
		return p.filter_exp;
	case PUMP_TREBLE_BOOST:
		return p.treble_boost;
	default:
		return -1.f;
	}
}

inline void pump_init(pump& p, double samplerate)
{
	p.samplerate = samplerate;
	pump_set_param(p, PUMP_ATTACK, p.attack_ms);
	pump_set_param(p, PUMP_RELEASE, p.release_ms);
}

template <typename sample>
inline void pump_process_block(pump& p, sample** audio, sample** sidechain, int frames)
{
	// highpass filter coefficients
	float hp_x = std::exp(-2.0 * M_PI * p.hp_filter / p.samplerate);
	float hp_a0 = 1.0 - hp_x;
	float hp_b1 = -hp_x;

	// top end boost filter coefficients
	float bst_x = exp(-2.0 * M_PI * 5000.0 / p.samplerate);
	float bst_a0 = 1.0 - bst_x;
	float bst_b1 = -bst_x;

	for (int i = 0; i < frames; i++) {

		sample input_l = audio[0][i];
		sample input_r = audio[1][i];

		sample sidechain_in = (sidechain[0][i] + sidechain[1][i]) / 2.0;

		// highpass filter sidechain signal
		p.filtered = hp_a0 * sidechain_in - hp_b1 * p.filtered;
		sidechain_in = sidechain_in - p.filtered;

		// rectify sidechain input for envelope following
		float link = std::fabs(sidechain_in);
		float linked_db = trnr::lin_2_db(link);

		// cut envelope below threshold
		float overshoot_db = linked_db - (p.threshold_db - 10.0);
		if (overshoot_db < 0.0) overshoot_db = 0.0;

		// process envelope
		if (overshoot_db > p.envelope_db) {
			p.envelope_db = overshoot_db + p.attack_coef * (p.envelope_db - overshoot_db);
		} else {
			p.envelope_db =
				overshoot_db + p.release_coef * (p.envelope_db - overshoot_db);
		}

		float slope = 1.f / p.ratio;

		// transfer function
		float gain_reduction_db = p.envelope_db * (slope - 1.0);
		float gain_reduction_lin = db_2_lin(gain_reduction_db);

		// compress left and right signals
		sample output_l = input_l * gain_reduction_lin;
		sample output_r = input_r * gain_reduction_lin;

		if (p.filter_exp > 0.f) {
			// one pole lowpass filter with envelope applied to frequency for pumping
			// effect
			float freq = p.filter_frq * pow(gain_reduction_lin, p.filter_exp);
			float lp_x = exp(-2.0 * M_PI * freq / p.samplerate);
			float lp_a0 = 1.0 - lp_x;
			float lp_b1 = -lp_x;
			p.filtered_l = lp_a0 * output_l - lp_b1 * p.filtered_l;
			p.filtered_r = lp_a0 * output_r - lp_b1 * p.filtered_r;
		}

		// top end boost
		p.boosted_l = bst_a0 * p.filtered_l - bst_b1 * p.boosted_l;
		p.boosted_r = bst_a0 * p.filtered_r - bst_b1 * p.boosted_r;
		output_l = p.filtered_l + (p.filtered_l - p.boosted_l) * p.treble_boost;
		output_r = p.filtered_r + (p.filtered_r - p.boosted_r) * p.treble_boost;

		// calculate makeup gain
		float makeup_lin = trnr::db_2_lin(p.makeup);

		audio[0][i] = input_l * gain_reduction_lin * makeup_lin;
		audio[1][i] = input_r * gain_reduction_lin * makeup_lin;
	}
}
} // namespace trnr

#pragma once
#include "audio_math.h"
#include "smoother.h"
#include <cmath>
#include <vector>

namespace trnr {

// Filter type enum
enum filter_type {
	LOWPASS = 0,
	HIGHPASS = 1
};

struct cascade_filter {
	filter_type type;
	int stages;				   // Number of cascaded stages
	double cutoff;			   // Cutoff frequency (Hz)
	double samplerate;		   // Sample rate (Hz)
	double alpha;			   // Filter coefficient
	std::vector<double> state; // State per stage
};

inline void cascade_filter_setup(cascade_filter& f, filter_type _type, int _stages, double _cutoff, double _samplerate)
{
	f.type = _type;
	f.stages = _stages;
	f.cutoff = _cutoff;
	f.samplerate = _samplerate;

	// alpha = iirAmount = exp(-2 * pi * cutoff / samplerate);
	double x = exp(-2.0 * M_PI * f.cutoff / f.samplerate);
	f.alpha = 1.0 - x;

	f.state.resize(f.stages, 0.0);
}

// Process one sample
inline double cascade_filter_process(cascade_filter& f, double input)
{
	double out = input;
	for (int i = 0; i < f.stages; ++i) {
		if (f.type == LOWPASS) {
			f.state[i] = (f.state[i] * (1.0 - f.alpha)) + (out * f.alpha);
			out = f.state[i];
		} else { // CASCADE_HIGHPASS
			f.state[i] = (f.state[i] * (1.0 - f.alpha)) + (out * f.alpha);
			out -= f.state[i];
		}
	}
	return out;
}

// 2nd order Butterworth biquad filter
struct butterworth {
	filter_type type;
	double cutoff;
	double a1, a2;
	double b0, b1, b2;
	double x1, x2; // previous inputs
	double y1, y2; // previous outputs
};

// Biquad coefficient calculation
inline void butterworth_biquad_coeffs(butterworth& b, double samplerate)
{
	double omega = 2.0 * M_PI * b.cutoff / samplerate;
	double sin_omega = sin(omega);
	double cos_omega = cos(omega);

	double Q = 1.0 / sqrt(2.0); // Butterworth Q for 2nd order
	double alpha = sin_omega / (2.0 * Q);
	double a0 = 1.0 + alpha;

	switch (b.type) {
	case LOWPASS:
		b.b0 = (1.0 - cos_omega) / 2.0 / a0;
		b.b1 = (1.0 - cos_omega) / a0;
		b.b2 = (1.0 - cos_omega) / 2.0 / a0;
		b.a1 = -2.0 * cos_omega / a0;
		b.a2 = (1.0 - alpha) / a0;
		break;
	case HIGHPASS:
		b.b0 = (1.0 + cos_omega) / 2.0 / a0;
		b.b1 = -(1.0 + cos_omega) / a0;
		b.b2 = (1.0 + cos_omega) / 2.0 / a0;
		b.a1 = -2.0 * cos_omega / a0;
		b.a2 = (1.0 - alpha) / a0;
		break;
	}
}

// Biquad sample processing
inline double butterworth_biquad_process(butterworth& b, double input)
{
	double y = b.b0 * input + b.b1 * b.x1 + b.b2 * b.x2 - b.a1 * b.y1 - b.a2 * b.y2;
	b.x2 = b.x1;
	b.x1 = input;
	b.y2 = b.y1;
	b.y1 = y;
	return y;
}

struct aw_filter {
	filter_type type;
	float amount;
	bool flip;
	double sampleLAA;
	double sampleLAB;
	double sampleLBA;
	double sampleLBB;
	double sampleLCA;
	double sampleLCB;
	double sampleLDA;
	double sampleLDB;
	double sampleLE;
	double sampleLF;
	double sampleLG;
	double samplerate;
};

inline void aw_filter_init(aw_filter& f, filter_type type, float amount, double samplerate)
{
	f.type = type;
	f.amount = amount;
	f.samplerate = samplerate;

	f.sampleLAA = 0.0;
	f.sampleLAB = 0.0;
	f.sampleLBA = 0.0;
	f.sampleLBB = 0.0;
	f.sampleLCA = 0.0;
	f.sampleLCB = 0.0;
	f.sampleLDA = 0.0;
	f.sampleLDB = 0.0;
	f.sampleLE = 0.0;
	f.sampleLF = 0.0;
	f.sampleLG = 0.0;

	f.flip = false;
}

inline void aw_filter_process_block(aw_filter& f, float* audio, int frames)
{
	double overallscale = 1.0;
	overallscale /= 44100.0;
	double compscale = overallscale;
	overallscale = f.samplerate;
	compscale = compscale * overallscale;
	bool engage = false;

	double iir_amt = 0.0;

	if (f.type == LOWPASS) {
		iir_amt = (((f.amount * f.amount * 15.0) + 1.0) * 0.0188) + 0.7;
		if (iir_amt > 1.0) iir_amt = 1.0;
		if (((f.amount * f.amount * 15.0) + 1.0) < 15.99) engage = true;
	} else if (f.type == HIGHPASS) {
		iir_amt = (((f.amount * f.amount * 1570.0) + 30.0) * 1.0) / overallscale;
		if (((f.amount * f.amount * 1570.0) + 30.0) > 30.01) engage = true;
	}

	for (int i = 0; i < frames; i++) {
		float input = audio[i];
		f.flip = !f.flip;

		if (engage) {
			switch (f.type) {
			case LOWPASS:
				if (f.flip) {
					f.sampleLAA = (f.sampleLAA * (1.0 - iir_amt)) + (input * iir_amt);
					input = f.sampleLAA;
					f.sampleLBA = (f.sampleLBA * (1.0 - iir_amt)) + (input * iir_amt);
					input = f.sampleLBA;
					f.sampleLCA = (f.sampleLCA * (1.0 - iir_amt)) + (input * iir_amt);
					input = f.sampleLCA;
					f.sampleLDA = (f.sampleLDA * (1.0 - iir_amt)) + (input * iir_amt);
					input = f.sampleLDA;
					f.sampleLE = (f.sampleLE * (1.0 - iir_amt)) + (input * iir_amt);
					input = f.sampleLE;
				} else {
					f.sampleLAB = (f.sampleLAB * (1.0 - iir_amt)) + (input * iir_amt);
					input = f.sampleLAB;
					f.sampleLBB = (f.sampleLBB * (1.0 - iir_amt)) + (input * iir_amt);
					input = f.sampleLBB;
					f.sampleLCB = (f.sampleLCB * (1.0 - iir_amt)) + (input * iir_amt);
					input = f.sampleLCB;
					f.sampleLDB = (f.sampleLDB * (1.0 - iir_amt)) + (input * iir_amt);
					input = f.sampleLDB;
					f.sampleLF = (f.sampleLF * (1.0 - iir_amt)) + (input * iir_amt);
					input = f.sampleLF;
				}
				f.sampleLG = (f.sampleLG * (1.0 - iir_amt)) + (input * iir_amt);
				input = (f.sampleLG * (1.0 - iir_amt)) + (input * iir_amt);
				break;
			case HIGHPASS:
				if (f.flip) {
					f.sampleLAA = (f.sampleLAA * (1.0 - iir_amt)) + (input * iir_amt);
					input -= f.sampleLAA;
					f.sampleLBA = (f.sampleLBA * (1.0 - iir_amt)) + (input * iir_amt);
					input -= f.sampleLBA;
					f.sampleLCA = (f.sampleLCA * (1.0 - iir_amt)) + (input * iir_amt);
					input -= f.sampleLCA;
					f.sampleLDA = (f.sampleLDA * (1.0 - iir_amt)) + (input * iir_amt);
					input -= f.sampleLDA;
				} else {
					f.sampleLAB = (f.sampleLAB * (1.0 - iir_amt)) + (input * iir_amt);
					input -= f.sampleLAB;
					f.sampleLBB = (f.sampleLBB * (1.0 - iir_amt)) + (input * iir_amt);
					input -= f.sampleLBB;
					f.sampleLCB = (f.sampleLCB * (1.0 - iir_amt)) + (input * iir_amt);
					input -= f.sampleLCB;
					f.sampleLDB = (f.sampleLDB * (1.0 - iir_amt)) + (input * iir_amt);
					input -= f.sampleLDB;
				}
				f.sampleLE = (f.sampleLE * (1.0 - iir_amt)) + (input * iir_amt);
				input -= f.sampleLE;
				f.sampleLF = (f.sampleLF * (1.0 - iir_amt)) + (input * iir_amt);
				input -= f.sampleLF;
				break;
			}
		}

		audio[i] = input;
	}
}

enum spliteq_mode {
	CASCADE_SUM,
	LINKWITZ_RILEY
};

struct spliteq {
	aw_filter lp_l, lp_r, hp_l, hp_r; // lowpass and highpass filters

	// cascaded filters
	cascade_filter bass_l, bass_r;
	cascade_filter treble_l, treble_r;

	// butterworth biquads
	butterworth bass1_l, bass2_l, bass1_r, bass2_r;
	// Mid: two cascaded highpass THEN two cascaded lowpass per channel
	butterworth mid_hp1_l, mid_hp2_l, mid_lp1_l, mid_lp2_l;
	butterworth mid_hp1_r, mid_hp2_r, mid_lp1_r, mid_lp2_r;
	// Treble: two cascaded highpass filters per channel
	butterworth treble1_l, treble2_l, treble1_r, treble2_r;

	double low_mid_crossover = 150.0;	// Hz
	double mid_high_crossover = 1700.0; // Hz

	// adjusted crossover frequencies for cascade filters
	double low_mid_crossover_adj = 150.0;
	double mid_high_crossover_adj = 1700.0;

	double samplerate = 48000.0;

	float bass_gain = 1.0f;
	float mid_gain = 1.0f;
	float treble_gain = 1.0f;

	// adjusted gain for cascade filters
	float bass_gain_adj = 1.0f;
	float mid_gain_adj = 1.0f;
	float treble_gain_adj = 1.0f;

	spliteq_mode current_mode = LINKWITZ_RILEY;
	spliteq_mode target_mode = LINKWITZ_RILEY;
	bool transitioning = false;
	smoother transition_smoother;
};

inline void spliteq_init(spliteq& eq, double samplerate, double low_mid_crossover, double mid_high_crossover)
{
	low_mid_crossover /= 2.0;
	mid_high_crossover /= 2.0;

	eq.samplerate = samplerate;

	eq.low_mid_crossover = low_mid_crossover;
	eq.mid_high_crossover = mid_high_crossover;
	eq.low_mid_crossover_adj = low_mid_crossover;
	eq.mid_high_crossover_adj = mid_high_crossover;

	// initialize lp/hp filters
	aw_filter_init(eq.lp_l, LOWPASS, 1.0f, samplerate);
	aw_filter_init(eq.lp_r, LOWPASS, 1.0f, samplerate);
	aw_filter_init(eq.hp_l, HIGHPASS, 0.0f, samplerate);
	aw_filter_init(eq.hp_r, HIGHPASS, 0.0f, samplerate);

	// init cascade filters
	cascade_filter_setup(eq.bass_l, LOWPASS, 2, low_mid_crossover, samplerate);
	cascade_filter_setup(eq.bass_r, LOWPASS, 2, low_mid_crossover, samplerate);
	cascade_filter_setup(eq.treble_l, HIGHPASS, 2, mid_high_crossover, samplerate);
	cascade_filter_setup(eq.treble_r, HIGHPASS, 2, mid_high_crossover, samplerate);

	// init butterworth filters
	// bass filters
	eq.bass1_l.type = LOWPASS;
	eq.bass1_l.cutoff = low_mid_crossover;
	eq.bass1_l.x1 = eq.bass1_l.x2 = eq.bass1_l.y1 = eq.bass1_l.y2 = 0.0;
	butterworth_biquad_coeffs(eq.bass1_l, samplerate);

	eq.bass2_l.type = LOWPASS;
	eq.bass2_l.cutoff = low_mid_crossover;
	eq.bass2_l.x1 = eq.bass2_l.x2 = eq.bass2_l.y1 = eq.bass2_l.y2 = 0.0;
	butterworth_biquad_coeffs(eq.bass2_l, samplerate);

	eq.bass1_r.type = LOWPASS;
	eq.bass1_r.cutoff = low_mid_crossover;
	eq.bass1_r.x1 = eq.bass1_r.x2 = eq.bass1_r.y1 = eq.bass1_r.y2 = 0.0;
	butterworth_biquad_coeffs(eq.bass1_r, samplerate);

	eq.bass2_r.type = LOWPASS;
	eq.bass2_r.cutoff = low_mid_crossover;
	eq.bass2_r.x1 = eq.bass2_r.x2 = eq.bass2_r.y1 = eq.bass2_r.y2 = 0.0;
	butterworth_biquad_coeffs(eq.bass2_r, samplerate);

	// mid filters (HPF x2, then LPF x2)
	eq.mid_hp1_l.type = HIGHPASS;
	eq.mid_hp1_l.cutoff = low_mid_crossover;
	eq.mid_hp1_l.x1 = eq.mid_hp1_l.x2 = eq.mid_hp1_l.y1 = eq.mid_hp1_l.y2 = 0.0;
	butterworth_biquad_coeffs(eq.mid_hp1_l, samplerate);

	eq.mid_hp2_l.type = HIGHPASS;
	eq.mid_hp2_l.cutoff = low_mid_crossover;
	eq.mid_hp2_l.x1 = eq.mid_hp2_l.x2 = eq.mid_hp2_l.y1 = eq.mid_hp2_l.y2 = 0.0;
	butterworth_biquad_coeffs(eq.mid_hp2_l, samplerate);

	eq.mid_lp1_l.type = LOWPASS;
	eq.mid_lp1_l.cutoff = mid_high_crossover;
	eq.mid_lp1_l.x1 = eq.mid_lp1_l.x2 = eq.mid_lp1_l.y1 = eq.mid_lp1_l.y2 = 0.0;
	butterworth_biquad_coeffs(eq.mid_lp1_l, samplerate);

	eq.mid_lp2_l.type = LOWPASS;
	eq.mid_lp2_l.cutoff = mid_high_crossover;
	eq.mid_lp2_l.x1 = eq.mid_lp2_l.x2 = eq.mid_lp2_l.y1 = eq.mid_lp2_l.y2 = 0.0;
	butterworth_biquad_coeffs(eq.mid_lp2_l, samplerate);

	eq.mid_hp1_r.type = HIGHPASS;
	eq.mid_hp1_r.cutoff = low_mid_crossover;
	eq.mid_hp1_r.x1 = eq.mid_hp1_r.x2 = eq.mid_hp1_r.y1 = eq.mid_hp1_r.y2 = 0.0;
	butterworth_biquad_coeffs(eq.mid_hp1_r, samplerate);

	eq.mid_hp2_r.type = HIGHPASS;
	eq.mid_hp2_r.cutoff = low_mid_crossover;
	eq.mid_hp2_r.x1 = eq.mid_hp2_r.x2 = eq.mid_hp2_r.y1 = eq.mid_hp2_r.y2 = 0.0;
	butterworth_biquad_coeffs(eq.mid_hp2_r, samplerate);

	eq.mid_lp1_r.type = LOWPASS;
	eq.mid_lp1_r.cutoff = mid_high_crossover;
	eq.mid_lp1_r.x1 = eq.mid_lp1_r.x2 = eq.mid_lp1_r.y1 = eq.mid_lp1_r.y2 = 0.0;
	butterworth_biquad_coeffs(eq.mid_lp1_r, samplerate);

	eq.mid_lp2_r.type = LOWPASS;
	eq.mid_lp2_r.cutoff = mid_high_crossover;
	eq.mid_lp2_r.x1 = eq.mid_lp2_r.x2 = eq.mid_lp2_r.y1 = eq.mid_lp2_r.y2 = 0.0;
	butterworth_biquad_coeffs(eq.mid_lp2_r, samplerate);

	// treble filters
	eq.treble1_l.type = HIGHPASS;
	eq.treble1_l.cutoff = mid_high_crossover;
	eq.treble1_l.x1 = eq.treble1_l.x2 = eq.treble1_l.y1 = eq.treble1_l.y2 = 0.0;
	butterworth_biquad_coeffs(eq.treble1_l, samplerate);

	eq.treble2_l.type = HIGHPASS;
	eq.treble2_l.cutoff = mid_high_crossover;
	eq.treble2_l.x1 = eq.treble2_l.x2 = eq.treble2_l.y1 = eq.treble2_l.y2 = 0.0;
	butterworth_biquad_coeffs(eq.treble2_l, samplerate);

	eq.treble1_r.type = HIGHPASS;
	eq.treble1_r.cutoff = mid_high_crossover;
	eq.treble1_r.x1 = eq.treble1_r.x2 = eq.treble1_r.y1 = eq.treble1_r.y2 = 0.0;
	butterworth_biquad_coeffs(eq.treble1_r, samplerate);

	eq.treble2_r.type = HIGHPASS;
	eq.treble2_r.cutoff = mid_high_crossover;
	eq.treble2_r.x1 = eq.treble2_r.x2 = eq.treble2_r.y1 = eq.treble2_r.y2 = 0.0;
	butterworth_biquad_coeffs(eq.treble2_r, samplerate);

	smoother_init(eq.transition_smoother, samplerate, 50.0f, 1.0f);
}

inline void spliteq_set_mode(spliteq& eq, spliteq_mode mode)
{
	if (eq.target_mode != mode) {
		eq.target_mode = mode;
		eq.transitioning = true;
		smoother_set_target(eq.transition_smoother, 0.0f);
	}
}

inline void linkwitz_riley_process(spliteq& eq, float**& audio, int& i)
{
	// left channel
	double input_l = audio[0][i];
	// bass
	double bass_l = butterworth_biquad_process(eq.bass1_l, input_l);
	bass_l = butterworth_biquad_process(eq.bass2_l, bass_l);
	// mid
	double mid_l = butterworth_biquad_process(eq.mid_hp1_l, input_l);
	mid_l = butterworth_biquad_process(eq.mid_hp2_l, mid_l);
	mid_l = butterworth_biquad_process(eq.mid_lp1_l, mid_l);
	mid_l = butterworth_biquad_process(eq.mid_lp2_l, mid_l);
	// treble
	double treble_l = butterworth_biquad_process(eq.treble1_l, input_l);
	treble_l = butterworth_biquad_process(eq.treble2_l, treble_l);

	// apply gains
	bass_l *= eq.bass_gain;
	mid_l *= eq.mid_gain;
	treble_l *= eq.treble_gain;

	// sum bands
	audio[0][i] = bass_l + mid_l + treble_l;

	// right channel
	double input_r = audio[1][i];
	double bass_r = butterworth_biquad_process(eq.bass1_r, input_r);
	bass_r = butterworth_biquad_process(eq.bass2_r, bass_r);

	double mid_r = butterworth_biquad_process(eq.mid_hp1_r, input_r);
	mid_r = butterworth_biquad_process(eq.mid_hp2_r, mid_r);
	mid_r = butterworth_biquad_process(eq.mid_lp1_r, mid_r);
	mid_r = butterworth_biquad_process(eq.mid_lp2_r, mid_r);

	double treble_r = butterworth_biquad_process(eq.treble1_r, input_r);
	treble_r = butterworth_biquad_process(eq.treble2_r, treble_r);

	bass_r *= eq.bass_gain;
	mid_r *= eq.mid_gain;
	treble_r *= eq.treble_gain;

	audio[1][i] = bass_r + mid_r + treble_r;
}

inline void cascade_sum_process(spliteq& eq, float**& audio, int& i)
{
	double input_l = audio[0][i];
	double input_r = audio[1][i];

	double bass_l = cascade_filter_process(eq.bass_l, input_l);
	double bass_r = cascade_filter_process(eq.bass_r, input_r);

	double treble_l = cascade_filter_process(eq.treble_l, input_l);
	double treble_r = cascade_filter_process(eq.treble_r, input_r);

	double mid_l = input_l - bass_l - treble_l;
	double mid_r = input_r - bass_r - treble_r;

	// apply gains
	bass_l *= eq.bass_gain_adj;
	bass_r *= eq.bass_gain_adj;
	mid_l *= eq.mid_gain_adj;
	mid_r *= eq.mid_gain_adj;
	treble_l *= eq.treble_gain_adj;
	treble_r *= eq.treble_gain_adj;

	// sum bands
	audio[0][i] = bass_l + mid_l + treble_l;
	audio[1][i] = bass_r + mid_r + treble_r;
}

inline void spliteq_process_block(spliteq& eq, float** audio, int frames)
{
	// highpass filters
	aw_filter_process_block(eq.hp_l, audio[0], frames);
	aw_filter_process_block(eq.hp_r, audio[1], frames);

	for (int i = 0; i < frames; i++) {
		float smooth_gain = 1.0f;

		if (eq.transitioning) {
			smooth_gain = smoother_process_sample(eq.transition_smoother);

			if (smooth_gain == 0.f) {
				smoother_set_target(eq.transition_smoother, 1.0);
				eq.current_mode = eq.target_mode;
			} else if (smooth_gain == 1.f) {
				eq.transitioning = false;
			}
		}

		if (eq.current_mode == LINKWITZ_RILEY) {
			linkwitz_riley_process(eq, audio, i);
		} else if (eq.current_mode == CASCADE_SUM) {
			cascade_sum_process(eq, audio, i);
		}

		audio[0][i] *= smooth_gain;
		audio[1][i] *= smooth_gain;
	}

	// lowpass filters
	aw_filter_process_block(eq.lp_l, audio[0], frames);
	aw_filter_process_block(eq.lp_r, audio[1], frames);
}

inline void spliteq_update(spliteq& eq, double hp_freq, double lp_freq, double low_mid_crossover,
						   double mid_high_crossover, double bass_gain, double mid_gain, double treble_gain)
{
	low_mid_crossover /= 2.0;
	mid_high_crossover /= 2.0;

	eq.bass_gain = db_2_lin(bass_gain);
	eq.mid_gain = db_2_lin(mid_gain);
	eq.treble_gain = db_2_lin(treble_gain);

	if (bass_gain > 0.f) {
		eq.bass_gain = eq.bass_gain_adj = db_2_lin(bass_gain * 0.85f);
		eq.low_mid_crossover_adj = low_mid_crossover;
	} else {
		eq.bass_gain_adj = db_2_lin(bass_gain);
		eq.low_mid_crossover_adj = low_mid_crossover * 2.0;
	}

	if (mid_gain > 0.0f) eq.mid_gain_adj = db_2_lin(mid_gain * 0.85f);
	else eq.mid_gain_adj = db_2_lin(mid_gain * 0.74f);

	if (treble_gain > 0.f) {
		eq.treble_gain_adj = db_2_lin(treble_gain * 1.1f);
		eq.mid_high_crossover_adj = mid_high_crossover;
	} else {
		eq.treble_gain_adj = db_2_lin(treble_gain);
		eq.mid_high_crossover_adj = mid_high_crossover / 2.0;
	}

	eq.low_mid_crossover = low_mid_crossover;
	eq.mid_high_crossover = mid_high_crossover;

	eq.hp_l.amount = hp_freq;
	eq.hp_r.amount = hp_freq;
	eq.lp_l.amount = lp_freq;
	eq.lp_r.amount = lp_freq;

	cascade_filter_setup(eq.bass_l, LOWPASS, 2, eq.low_mid_crossover_adj, eq.samplerate);
	cascade_filter_setup(eq.bass_r, LOWPASS, 2, eq.low_mid_crossover_adj, eq.samplerate);
	cascade_filter_setup(eq.treble_l, HIGHPASS, 2, eq.mid_high_crossover_adj, eq.samplerate);
	cascade_filter_setup(eq.treble_r, HIGHPASS, 2, eq.mid_high_crossover_adj, eq.samplerate);

	eq.bass1_l.cutoff = low_mid_crossover;
	butterworth_biquad_coeffs(eq.bass1_l, eq.samplerate);
	eq.bass2_l.cutoff = low_mid_crossover;
	butterworth_biquad_coeffs(eq.bass2_l, eq.samplerate);
	eq.bass1_r.cutoff = low_mid_crossover;
	butterworth_biquad_coeffs(eq.bass1_r, eq.samplerate);
	eq.bass2_r.cutoff = low_mid_crossover;
	butterworth_biquad_coeffs(eq.bass2_r, eq.samplerate);

	eq.mid_hp1_l.cutoff = low_mid_crossover;
	butterworth_biquad_coeffs(eq.mid_hp1_l, eq.samplerate);
	eq.mid_hp2_l.cutoff = low_mid_crossover;
	butterworth_biquad_coeffs(eq.mid_hp2_l, eq.samplerate);
	eq.mid_lp1_l.cutoff = mid_high_crossover;
	butterworth_biquad_coeffs(eq.mid_lp1_l, eq.samplerate);
	eq.mid_lp2_l.cutoff = mid_high_crossover;
	butterworth_biquad_coeffs(eq.mid_lp2_l, eq.samplerate);

	eq.mid_hp1_r.cutoff = low_mid_crossover;
	butterworth_biquad_coeffs(eq.mid_hp1_r, eq.samplerate);
	eq.mid_hp2_r.cutoff = low_mid_crossover;
	butterworth_biquad_coeffs(eq.mid_hp2_r, eq.samplerate);
	eq.mid_lp1_r.cutoff = mid_high_crossover;
	butterworth_biquad_coeffs(eq.mid_lp1_r, eq.samplerate);
	eq.mid_lp2_r.cutoff = mid_high_crossover;
	butterworth_biquad_coeffs(eq.mid_lp2_r, eq.samplerate);

	eq.treble1_l.cutoff = mid_high_crossover;
	butterworth_biquad_coeffs(eq.treble1_l, eq.samplerate);
	eq.treble2_l.cutoff = mid_high_crossover;
	butterworth_biquad_coeffs(eq.treble2_l, eq.samplerate);
	eq.treble1_r.cutoff = mid_high_crossover;
	butterworth_biquad_coeffs(eq.treble1_r, eq.samplerate);
	eq.treble2_r.cutoff = mid_high_crossover;
	butterworth_biquad_coeffs(eq.treble2_r, eq.samplerate);
}

inline void spliteq_update(spliteq& eq, double bass_gain, double mid_gain, double treble_gain)
{
	trnr::spliteq_update(eq, eq.hp_l.amount, eq.lp_l.amount, eq.low_mid_crossover * 2.0, eq.mid_high_crossover * 2.0,
						 bass_gain, mid_gain, treble_gain);
}
} // namespace trnr

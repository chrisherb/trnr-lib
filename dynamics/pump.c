#include <cmath>
#include "../util/sample.h"
#include "../util/audio_math.c"

typedef struct {
    // parameters
    double samplerate;
	float threshold_db;
	float attack_ms;
	float release_ms;
	float hp_filter;
	float ratio;
	float filter_frq;
	float filter_exp;
	float treble_boost;

    // internal state
	trnr_sample filtered;
	trnr_sample filtered_l;
	trnr_sample filtered_r;
	trnr_sample boosted_l;
	trnr_sample boosted_r;
	float envelope_db;
} trnr_pump;

trnr_pump* trnr_pump_create()
{
    trnr_pump* pump = (trnr_pump*)malloc(sizeof(trnr_pump));
    if (pump == NULL) return NULL;

    pump->samplerate = 44100.0;
    pump->threshold_db = 0.f;
    pump->attack_ms = 10.f;
    pump->release_ms = 100.f;
    pump->hp_filter = 20.f;
    pump->ratio = 4.f;
    pump->filter_frq = 100.f;
    pump->filter_exp = 0.f;
    pump->treble_boost = 0.f;

    pump->filtered = 0.f;
    pump->filtered_l = 0.f;
    pump->filtered_r = 0.f;
    pump->boosted_l = 0.f;
    pump->boosted_r = 0.f;
    pump->envelope_db = 0.f;

    return pump;
}

void trnr_pump_process_block(trnr_pump* pump, trnr_sample** audio, trnr_sample** sidechain, int frames)
{
    // highpass filter coefficients
    float hp_x = exp(-2.0 * M_PI * pump->hp_filter / pump->samplerate);
    float hp_a0 = 1.0 - hp_x;
    float hp_b1 = -hp_x;

    // top end boost filter coefficients
    float bst_x = exp(-2.0 * M_PI * 5000.0 / pump->samplerate);
    float bst_a0 = 1.0 - bst_x;
    float bst_b1 = -bst_x;

    // attack and release coefficients
    float attack_coef = exp(-1000.0 / (pump->attack_ms * pump->samplerate));
    float release_coef = exp(-1000.0 / (pump->release_ms * pump->samplerate));

    for (int i = 0; i < frames; i++) {

        trnr_sample input_l = audio[0][i];
        trnr_sample input_r = audio[1][i];

        trnr_sample sidechain_in = (sidechain[0][i] + sidechain[1][i]) / 2.0;

        // highpass filter sidechain signal
        pump->filtered = hp_a0 * sidechain_in - hp_b1 * pump->filtered;
        sidechain_in = sidechain_in - pump->filtered;

        // rectify sidechain input for envelope following
        float link = fabs(sidechain_in);
        float linked_db = trnr_lin_2_db(link);

        // cut envelope below threshold
        float overshoot_db = linked_db - (pump->threshold_db - 10.0);
        if (overshoot_db < 0.0) overshoot_db = 0.0;

        // process envelope
        if (overshoot_db > pump->envelope_db) {
            pump->envelope_db = overshoot_db + attack_coef * (pump->envelope_db - overshoot_db);
        } else {
            pump->envelope_db = overshoot_db + release_coef * (pump->envelope_db - overshoot_db);
        }

        float slope = 1.f / pump->ratio;

        // transfer function
        float gain_reduction_db = pump->envelope_db * (slope - 1.0);
        float gain_reduction_lin = trnr_db_2_lin(gain_reduction_db);

        // compress left and right signals
        trnr_sample output_l = input_l * gain_reduction_lin;
        trnr_sample output_r = input_r * gain_reduction_lin;

        if (pump->filter_exp > 0.f) {
            // one pole lowpass filter with envelope applied to frequency for pumping effect
            float freq = pump->filter_frq * pow(gain_reduction_lin, pump->filter_exp);
            float lp_x = exp(-2.0 * M_PI * freq / pump->samplerate);
            float lp_a0 = 1.0 - lp_x;
            float lp_b1 = -lp_x;
            pump->filtered_l = lp_a0 * output_l - lp_b1 * pump->filtered_l;
            pump->filtered_r = lp_a0 * output_r - lp_b1 * pump->filtered_r;
        }

        // top end boost
        pump->boosted_l = bst_a0 * pump->filtered_l - bst_b1 * pump->boosted_l;
        pump->boosted_r = bst_a0 * pump->filtered_r - bst_b1 * pump->boosted_r;
        output_l = pump->filtered_l + (pump->filtered_l - pump->boosted_l) * pump->treble_boost;
        output_r = pump->filtered_r + (pump->filtered_r - pump->boosted_r) * pump->treble_boost;

        // calculate makeup gain
        float makeup_lin = trnr_db_2_lin(-pump->threshold_db / 5.f);

        audio[0][i] = input_l * gain_reduction_lin * makeup_lin;
        audio[1][i] = input_r * gain_reduction_lin * makeup_lin;
    }
}
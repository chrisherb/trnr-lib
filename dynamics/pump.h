#pragma once

namespace trnr {
template <typename sample>
class pump {
public:
	pump()
		: attack_ms(10.f)
		, release_ms(100.f)
		, hp_filter(80.0f)
		, threshold_db(0.f)
		, ratio(1000.0f)
		, filter_frq(40000.f)
		, filter_exp(0.f)
		, treble_boost(0.f)
	{
		set_samplerate(44100);
	}

	void set_samplerate(double _samplerate)
	{
		samplerate = _samplerate;
		set_attack(attack_ms);
		set_release(release_ms);
	}

	void set_ratio(float value) { ratio = value; }

	/* set threshold in db */
	void set_threshold(float value) { threshold_db = value; }

	/* set attack in ms */
	void set_attack(float value)
	{
		attack_ms = value;
		attack_coef = exp(-1000.0 / (attack_ms * samplerate));
	}

	/* set release in ms */
	void set_release(float value)
	{
		release_ms = value;
		release_coef = exp(-1000.0 / (release_ms * samplerate));
	}

	/* lowpass filter frequency in hz */
	void set_filter_frq(float value) { filter_frq = value; }

	/* accentuates the filter pumping effect */
	void set_filter_exp(float value) { filter_exp = value; }

	void process_block(sample** audio, sample** sidechain, int frames)
	{
		// highpass filter coefficients
		float hp_x = std::exp(-2.0 * M_PI * hp_filter / samplerate);
		float hp_a0 = 1.0 - hp_x;
		float hp_b1 = -hp_x;

		// top end boost filter coefficients
		float bst_x = exp(-2.0 * M_PI * 5000.0 / samplerate);
		float bst_a0 = 1.0 - bst_x;
		float bst_b1 = -bst_x;

		for (int i = 0; i < frames; i++) {

			sample input_l = audio[0][i];
			sample input_r = audio[1][i];

			sample sidechain_in = (sidechain[0][i] + sidechain[1][i]) / 2.0;

			// highpass filter sidechain signal
			filtered = hp_a0 * sidechain_in - hp_b1 * filtered;
			sidechain_in = sidechain_in - filtered;

			// rectify sidechain input for envelope following
			float link = std::fabs(sidechain_in);
			float linked_db = trnr::lin_2_db(link);

			// cut envelope below threshold
			float overshoot_db = linked_db - (threshold_db - 10.0);
			if (overshoot_db < 0.0) overshoot_db = 0.0;

			// process envelope
			if (overshoot_db > envelope_db) {
				envelope_db = overshoot_db + attack_coef * (envelope_db - overshoot_db);
			} else {
				envelope_db = overshoot_db + release_coef * (envelope_db - overshoot_db);
			}

			float slope = 1.f / ratio;

			// transfer function
			float gain_reduction_db = envelope_db * (slope - 1.0);
			float gain_reduction_lin = trnr::db_2_lin(gain_reduction_db);

			// compress left and right signals
			sample output_l = input_l * gain_reduction_lin;
			sample output_r = input_r * gain_reduction_lin;

			if (filter_exp > 0.f) {
				// one pole lowpass filter with envelope applied to frequency for pumping effect
				float freq = filter_frq * pow(gain_reduction_lin, filter_exp);
				float lp_x = exp(-2.0 * M_PI * freq / samplerate);
				float lp_a0 = 1.0 - lp_x;
				float lp_b1 = -lp_x;
				filtered_l = lp_a0 * output_l - lp_b1 * filtered_l;
				filtered_r = lp_a0 * output_r - lp_b1 * filtered_r;
			}

			// top end boost
			boosted_l = bst_a0 * filtered_l - bst_b1 * boosted_l;
			boosted_r = bst_a0 * filtered_r - bst_b1 * boosted_r;
			output_l = filtered_l + (filtered_l - boosted_l) * treble_boost;
			output_r = filtered_r + (filtered_r - boosted_r) * treble_boost;

			// calculate makeup gain
			float makeup_lin = trnr::db_2_lin(-threshold_db / 5.f);

			audio[0][i] = input_l * gain_reduction_lin * makeup_lin;
			audio[1][i] = input_r * gain_reduction_lin * makeup_lin;
		}
	}

private:
	double samplerate;
	float threshold_db = 0.f;
	float attack_ms;
	float release_ms;
	float hp_filter;
	float ratio;
	float filter_frq;
	float filter_exp;
	float treble_boost;

	sample filtered;
	sample filtered_l = 0;
	sample filtered_r = 0;
	sample boosted_l = 0;
	sample boosted_r = 0;
	float attack_coef;
	float release_coef;
	float envelope_db = 0;
};
} // namespace trnr
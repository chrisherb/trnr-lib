#pragma once
#include <cmath>
#include <cstdint>
#include <stdlib.h>

namespace trnr {
// ulaw compansion based on code by Chris Johnson
class ulaw {
public:
	ulaw()
	{
		fpd_l = 1.0;
		while (fpd_l < 16386) fpd_l = rand() * UINT32_MAX;
		fpd_r = 1.0;
		while (fpd_r < 16386) fpd_r = rand() * UINT32_MAX;
	}

	void encode_samples(double& input_sample_l, double& input_sample_r)
	{

		// ulaw encoding
		static int noisesource_l = 0;
		static int noisesource_r = 850010;
		int residue;
		double applyresidue;

		noisesource_l = noisesource_l % 1700021;
		noisesource_l++;
		residue = noisesource_l * noisesource_l;
		residue = residue % 170003;
		residue *= residue;
		residue = residue % 17011;
		residue *= residue;
		residue = residue % 1709;
		residue *= residue;
		residue = residue % 173;
		residue *= residue;
		residue = residue % 17;
		applyresidue = residue;
		applyresidue *= 0.00000001;
		applyresidue *= 0.00000001;
		input_sample_l += applyresidue;
		if (input_sample_l < 1.2e-38 && -input_sample_l < 1.2e-38) { input_sample_l -= applyresidue; }

		noisesource_r = noisesource_r % 1700021;
		noisesource_r++;
		residue = noisesource_r * noisesource_r;
		residue = residue % 170003;
		residue *= residue;
		residue = residue % 17011;
		residue *= residue;
		residue = residue % 1709;
		residue *= residue;
		residue = residue % 173;
		residue *= residue;
		residue = residue % 17;
		applyresidue = residue;
		applyresidue *= 0.00000001;
		applyresidue *= 0.00000001;
		input_sample_r += applyresidue;
		if (input_sample_r < 1.2e-38 && -input_sample_r < 1.2e-38) { input_sample_r -= applyresidue; }

		if (input_sample_l > 1.0) input_sample_l = 1.0;
		if (input_sample_l < -1.0) input_sample_l = -1.0;

		if (input_sample_r > 1.0) input_sample_r = 1.0;
		if (input_sample_r < -1.0) input_sample_r = -1.0;

		if (input_sample_l > 0) input_sample_l = log(1.0 + (255 * fabs(input_sample_l))) / log(256);
		if (input_sample_l < 0) input_sample_l = -log(1.0 + (255 * fabs(input_sample_l))) / log(256);

		if (input_sample_r > 0) input_sample_r = log(1.0 + (255 * fabs(input_sample_r))) / log(256);
		if (input_sample_r < 0) input_sample_r = -log(1.0 + (255 * fabs(input_sample_r))) / log(256);
	}

	void decode_samples(double& input_sample_l, double& input_sample_r)
	{

		// ulaw decoding
		if (fabs(input_sample_l) < 1.18e-23) input_sample_l = fpd_l * 1.18e-17;
		if (fabs(input_sample_r) < 1.18e-23) input_sample_r = fpd_r * 1.18e-17;

		if (input_sample_l > 1.0) input_sample_l = 1.0;
		if (input_sample_l < -1.0) input_sample_l = -1.0;

		if (input_sample_r > 1.0) input_sample_r = 1.0;
		if (input_sample_r < -1.0) input_sample_r = -1.0;

		if (input_sample_l > 0) input_sample_l = (pow(256, fabs(input_sample_l)) - 1.0) / 255;
		if (input_sample_l < 0) input_sample_l = -(pow(256, fabs(input_sample_l)) - 1.0) / 255;

		if (input_sample_r > 0) input_sample_r = (pow(256, fabs(input_sample_r)) - 1.0) / 255;
		if (input_sample_r < 0) input_sample_r = -(pow(256, fabs(input_sample_r)) - 1.0) / 255;

		// 64 bit stereo floating point dither
		fpd_l ^= fpd_l << 13;
		fpd_l ^= fpd_l >> 17;
		fpd_l ^= fpd_l << 5;
		fpd_r ^= fpd_r << 13;
		fpd_r ^= fpd_r >> 17;
		fpd_r ^= fpd_r << 5;
	}

private:
	uint32_t fpd_l;
	uint32_t fpd_r;
};
} // namespace trnr
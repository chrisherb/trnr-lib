#pragma once
#define _USE_MATH_DEFINES
#include <array>
#include <math.h>

namespace trnr {
class chebyshev {
public:
	void set_samplerate(double _samplerate) { samplerate = _samplerate; }

	void process_sample(double& input, double frequency)
	{

		if (frequency >= 20000.f) { frequency = 20000.f; }

		// First calculate the prewarped digital frequency :
		auto K = tanf(M_PI * frequency / samplerate);

		// Now we calc some Coefficients :
		auto sg = sinh(passband_ripple);
		auto cg = cosh(passband_ripple);
		cg *= cg;

		std::array<double, 4> coeff;
		coeff[0] = 1 / (cg - 0.85355339059327376220042218105097);
		coeff[1] = K * coeff[0] * sg * 1.847759065022573512256366378792;
		coeff[2] = 1 / (cg - 0.14644660940672623779957781894758);
		coeff[3] = K * coeff[2] * sg * 0.76536686473017954345691996806;

		K *= K; // (just to optimize it a little bit)

		// Calculate the first biquad:
		a0 = 1 / (coeff[1] + K + coeff[0]);
		a1 = 2 * (coeff[0] - K) * a0;
		a2 = (coeff[1] - K - coeff[0]) * a0;
		b0 = a0 * K;
		b1 = 2 * b0;
		b2 = b0;

		// Calculate the second biquad:
		a3 = 1 / (coeff[3] + K + coeff[2]);
		a4 = 2 * (coeff[2] - K) * a3;
		a5 = (coeff[3] - K - coeff[2]) * a3;
		b3 = a3 * K;
		b4 = 2 * b3;
		b5 = b3;

		// Then calculate the output as follows:
		auto Stage1 = b0 * input + state0;
		state0 = b1 * input + a1 * Stage1 + state1;
		state1 = b2 * input + a2 * Stage1;
		input = b3 * Stage1 + state2;
		state2 = b4 * Stage1 + a4 * input + state3;
		state3 = b5 * Stage1 + a5 * input;
	}

private:
	double samplerate = 0;
	double a0 = 0;
	double a1 = 0;
	double a2 = 0;
	double a3 = 0;
	double a4 = 0;
	double a5 = 0;
	double b0 = 0;
	double b1 = 0;
	double b2 = 0;
	double b3 = 0;
	double b4 = 0;
	double b5 = 0;
	double state0 = 0;
	double state1 = 0;
	double state2 = 0;
	double state3 = 0;
	double passband_ripple = 1;
};
} // namespace trnr
#pragma once

#include <array>
#include <vector>

#include "../filter/chebyshev.h"

namespace trnr {

template<typename sample>
class oversampler {
public:
	oversampler()
	{
		buffer[0].reserve(num_samples);
		buffer[1].reserve(num_samples);
	}

	~oversampler() { delete[] ptrs; }

	void init(double _samplerate, int _ratio)
	{
		samplerate = _samplerate * _ratio;

		filter_freq = _samplerate * 0.5 - 4000;

		lowpass_in1.reset(samplerate, filter_freq);
		lowpass_in2.reset(samplerate, filter_freq);
		lowpass_out1.reset(samplerate, filter_freq);
		lowpass_out2.reset(samplerate, filter_freq);

		ratio = _ratio;
	}

	sample** upsample(sample** _inputs, int _blocksize)
	{
		num_samples = _blocksize;
		required_blocksize = _blocksize * ratio;

		if (required_blocksize > current_blocksize) {
			// resize buffer
			buffer[0].resize(required_blocksize);
			buffer[1].resize(required_blocksize);

			current_blocksize = required_blocksize;
		}

		for (int i = 0; i < _blocksize; ++i) {
			const int adjusted_index = i * ratio;

			buffer[0][adjusted_index] = _inputs[0][i];
			buffer[1][adjusted_index] = _inputs[1][i];

			for (int j = 1; j < ratio; ++j) {
				buffer[0][adjusted_index + j] = 0.0f;
				buffer[1][adjusted_index + j] = 0.0f;
			}
		}

		if (ratio > 1) {
			lowpass_in1.process_block(buffer[0].data(), required_blocksize);
			lowpass_in2.process_block(buffer[1].data(), required_blocksize);

			// compensate volume loss
			for (int i = 0; i < required_blocksize; ++i) {
				buffer[0][i] *= ratio;
				buffer[1][i] *= ratio;
			}
		}

		ptrs[0] = buffer[0].data();
		ptrs[1] = buffer[1].data();

		return ptrs;
	}

	void downsample(sample** _outputs)
	{
		if (ratio > 1) {
			lowpass_out1.process_block(buffer[0].data(), required_blocksize);
			lowpass_out2.process_block(buffer[1].data(), required_blocksize);
		}

		for (int i = 0; i < num_samples; ++i) {
			_outputs[0][i] = buffer[0][i * ratio];
			_outputs[1][i] = buffer[1][i * ratio];
		}
	}

private:
	int ratio = 1;
	double samplerate = 48000;
	int num_samples = 256;

	float filter_freq = 20000.f;

	int current_blocksize = num_samples;
	int required_blocksize = num_samples;

	chebyshev lowpass_in1 {samplerate, 20000};
	chebyshev lowpass_in2 {samplerate, 20000};
	chebyshev lowpass_out1 {samplerate, 20000};
	chebyshev lowpass_out2 {samplerate, 20000};

	std::array<std::vector<sample>, 2> buffer;
	sample** ptrs = new sample*[2];
};
} // namespace trnr
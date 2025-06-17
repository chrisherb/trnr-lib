#pragma once

#include "audio_buffer.h"
#include <vector>

namespace trnr {
template <typename t_sample>
struct ivoice {
	virtual ~ivoice() = default;
	virtual void process_samples(t_sample** _outputs, int _start_index, int _block_size,
								 std::vector<audio_buffer<t_sample>> _modulators = {}) = 0;
	virtual bool is_busy() = 0;
	virtual void set_samplerate(double samplerate) = 0;
	virtual void note_on(int _note, float _velocity) = 0;
	virtual void note_off() = 0;
	virtual void modulate_pitch(float _pitch) = 0;
};

// check if a template derives from ivoice
template <class derived, typename sample>
struct is_convertible {
	template <class T>
	static char test(T*);

	template <class T>
	static double test(...);

	static const bool value = sizeof(test<ivoice<sample>>(static_cast<derived*>(0))) == 1;
};
} // namespace trnr
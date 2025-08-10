#pragma once
#include "audio_buffer.h"
#include "midi_event.h"
#include "voice_allocator.h"
#include <cstddef>
#include <cstring>
#include <vector>

namespace trnr {

// a generic midi synth base class with sample accurate event handling.
// the templated type t_voice must derive from ivoice
template <typename t_voice, typename t_sample>
class midi_synth : public voice_allocator<t_voice, t_sample> {
public:
	std::vector<midi_event> m_event_queue;
	int m_block_size;
	bool m_voices_active;

	midi_synth(size_t num_voices = 1)
		: m_voices_active {false}
		, voice_allocator<t_voice, t_sample>(num_voices)
	{
	}

	void set_samplerate_blocksize(double _samplerate, int _block_size)
	{
		m_block_size = _block_size;
		voice_allocator<t_voice, t_sample>::set_samplerate(_samplerate);
	}

	void process_block(t_sample** _outputs, int _n_frames, std::vector<audio_buffer<t_sample>> _modulators = {})
	{
		// clear outputs
		for (auto i = 0; i < 2; i++) { memset(_outputs[i], 0, _n_frames * sizeof(t_sample)); }

		// sample accurate event handling based on the iPlug2 synth by Oli Larkin
		if (m_voices_active || !m_event_queue.empty()) {
			int block_size = m_block_size;
			int samples_remaining = _n_frames;
			int start_index = 0;

			while (samples_remaining > 0) {

				if (samples_remaining < block_size) block_size = samples_remaining;

				while (!m_event_queue.empty()) {
					midi_event event = m_event_queue.front();

					// we assume the messages are in chronological order. If we find one later than the current block we
					// are done.
					if (event.offset > start_index + block_size) break;

					// send performance messages to the voice allocator
					// message offset is relative to the start of this process_samples() block
					event.offset -= start_index;
					voice_allocator<t_voice, t_sample>::add_event(event);

					m_event_queue.erase(m_event_queue.begin());
				}

				voice_allocator<t_voice, t_sample>::process_samples(_outputs, start_index, block_size, _modulators);

				samples_remaining -= block_size;
				start_index += block_size;
			}

			m_voices_active = voice_allocator<t_voice, t_sample>::voices_active();

			flush_event_queue(_n_frames);
		} else {
			for (int s = 0; s < _n_frames; s++) {
				_outputs[0][s] = 0.;
				_outputs[1][s] = 0.;
			}
		}
	}

	void add_event(midi_event event)
	{
		if (event.type == midi_event_type::note_on) m_voices_active = true;

		m_event_queue.push_back(event);
	}

	void flush_event_queue(int frames)
	{
		for (int i = 0; i < m_event_queue.size(); i++) { m_event_queue.at(i).offset -= frames; }
	}
};
} // namespace trnr

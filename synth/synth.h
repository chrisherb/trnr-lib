#pragma once

#include <cstddef>
#include <memory>
#include <vector>

using namespace std;

namespace trnr {

enum m_event_type {
	note_on = 0,
	note_off,
	pitch_wheel,
	mod_wheel
};

struct m_event {
	m_event_type type;
	int offset;
	int midi_note;
	float velocity;
	double data;
};

template <typename voice>
struct synth {
	vector<shared_ptr<voice>> voice_ptrs;
	int active_voice_count;
};

template <typename voice>
void synth_init(const synth<voice>& s, size_t num_voices, double samplerate)
{
	s.active_voice_count = 0;
	s.voice_ptrs.reserve(num_voices);
	for (size_t i = 0; i < num_voices; ++i) { s.voice_ptrs.emplace_back(make_shared<voice>()); }
}

template <typename voice, typename sample>
void synth_process_block(const synth<voice>& s, sample** audio, const vector<m_event>& midi_events, int frames)
{
	int current_frame = 0;
	int event_index = 0;

	for (const auto& ev : midi_events) {

		int event_frame = ev.offset;
		int voice_index = -1;

		switch (ev.type) {
		case note_on:
			for (size_t i = 0; i < s.active_voice_count; ++i) {
				// attempt to find a free voice
				if (!s.voice_ptrs[i]->is_busy) { voice_index = i; }
			}
			// if no free voice is found, steal one
			if (voice_index < 0) {
				// Try to find a voice that is not gated (not playing a note)
				for (size_t i = 0; i < s.active_voice_count; ++i) {
					if (!s.voice_ptrs[i]->gate) { voice_index = i; }
				}
				// If all voices are gated, steal one round-robin
				voice_index = s.index_to_steal;
				s.index_to_steal = (s.index_to_steal + 1) % s.active_voice_count;
			}

			// If a voice is found, remember index
			// if (voice_index >= 0) { v->note_on(ev.midi_note, ev.velocity); }
			break;
		case note_off:
			break;
		case pitch_wheel:
			break;
		case mod_wheel:
			break;
		}

		// process all voices from current frame to event frame
		for (int i = 0; i < s.active_voice_count; i++) {
			auto& v = s.voice_ptrs[i];
			if (i == voice_index) { v.midi_event = ev; }
			if (current_frame < event_frame) { v->process_block(audio, current_frame, event_frame); }
		}

		current_frame = event_frame;
	}
}
} // namespace trnr

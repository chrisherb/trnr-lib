#pragma once

#include "audio_buffer.h"
#include <array>
#include <cstddef>
#include <vector>

using namespace std;

namespace trnr {

enum midi_event_type {
	note_on = 0,
	note_off,
	pitch_wheel,
	mod_wheel
};

struct midi_event {
	midi_event_type type;
	int offset;
	int midi_note;
	float velocity;
	double data;
};

constexpr size_t MAX_VOICES = 8;
constexpr size_t MAX_EVENTS_PER_VOICE = 32;

template <typename t_voice, typename t_sample>
void voice_process_block(t_voice& v, t_sample** frames, size_t num_frames, midi_event* events, size_t num_events,
						 const vector<audio_buffer<t_sample>>& mods = {});

template <typename t_voice>
struct synth {
	array<t_voice, MAX_VOICES> voices;
	array<array<midi_event, MAX_EVENTS_PER_VOICE>, MAX_VOICES> voice_events;
	array<size_t, MAX_VOICES> counts;
	int active_voice_count = 1;
	size_t index_to_steal = 0;
};

template <typename t_voice>
void synth_init(synth<t_voice>& s, double samplerate)
{
	for (size_t i = 0; i < MAX_VOICES; ++i) {
		s.voices[i] = t_voice();
		s.voices[i].voice_init(samplerate);
		s.counts[i] = 0;
	}
}

template <typename t_voice, typename t_sample>
void synth_process_block(synth<t_voice>& s, t_sample** frames, int num_frames, const vector<midi_event>& midi_events,
						 const vector<audio_buffer<t_sample>>& mods = {})
{
	// reset voice events and counts
	for (int i = 0; i < MAX_VOICES; i++) {
		s.voice_events[i].fill(midi_event {});
		s.counts[i] = 0;
	}

	for (const auto& ev : midi_events) {
		switch (ev.type) {
		case note_on: {
			bool found = false;
			// attempt to find a free voice
			for (size_t i = 0; i < s.active_voice_count; ++i) {
				if (!s.voices[i].is_busy) {
					s.voice_events[i][s.counts[i]++] = ev;
					found = true;
					break;
				}
			}

			if (found) break;

			// try to find a voice that is not gated
			for (size_t i = 0; i < s.active_voice_count; ++i) {
				if (!s.voices[i].gate) {
					s.voice_events[i][s.counts[i]++] = ev;
					found = true;
					break;
				}
			}

			if (found) break;

			// if all voices are gated, steal one round-robin
			s.voice_events[s.index_to_steal][s.counts[s.index_to_steal]++] = ev;
			s.index_to_steal++;
			if (s.index_to_steal >= s.active_voice_count) s.index_to_steal = 0;
			break;
		}
		case note_off: {
			for (size_t i = 0; i < s.active_voice_count; ++i) {
				if (s.voices[i].midi_note == ev.midi_note) s.voice_events[i][s.counts[i]++] = ev;
			}
			break;
		}
		case pitch_wheel:
		case mod_wheel: {
			for (size_t i = 0; i < s.active_voice_count; ++i) { s.voice_events[i][s.counts[i]++] = ev; }
			break;
		}
		}
	}

	for (size_t i = 0; i < s.active_voice_count; ++i) {
		auto& v = s.voices[i];
		auto& events = s.voice_events[i].data();
		size_t num_events = s.counts[i];
		voice_process_block(v, frames, num_frames, events, num_events, mods);
	}
}
} // namespace trnr

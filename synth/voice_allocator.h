#pragma once

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

#ifndef MAX_VOICES
#define MAX_VOICES 16
#endif

#ifndef MAX_EVENTS_PER_VOICE
#define MAX_EVENTS_PER_VOICE 32
#endif

struct voice_state {
	int midi_note;
	bool is_busy;
	bool gate;
	bool trigger;
	float velocity;
	array<midi_event, MAX_EVENTS_PER_VOICE> events;
};

struct voice_allocator {
	array<voice_state, MAX_VOICES> voices;
	array<size_t, MAX_EVENTS_PER_VOICE> counts;
	int active_voice_count = 1;
	size_t index_to_steal = 0;
};

inline void voice_allocator_init(voice_allocator& va)
{
	for (size_t i = 0; i < MAX_VOICES; ++i) { va.counts[i] = 0; }
}

inline void voice_allocator_process_block(voice_allocator& va, const vector<midi_event>& midi_events)
{
	// reset voice events and counts
	for (int i = 0; i < MAX_VOICES; i++) {
		for (int j = 0; j < MAX_EVENTS_PER_VOICE; j++) { va.voices[i].events[j] = midi_event {}; }
	}

	for (const auto& ev : midi_events) {
		switch (ev.type) {
		case note_on: {
			bool found = false;
			// attempt to find a free voice
			for (size_t i = 0; i < va.active_voice_count; ++i) {
				if (!va.voices[i].is_busy) {
					va.voices[i].events[va.counts[i]++] = ev;
					found = true;
					break;
				}
			}

			if (found) break;

			// try to find a voice that is not gated
			for (size_t i = 0; i < va.active_voice_count; ++i) {
				if (!va.voices[i].gate) {
					va.voices[i].events[va.counts[i]++] = ev;
					found = true;
					break;
				}
			}

			if (found) break;

			// if all voices are gated, steal one round-robin
			va.voices[va.index_to_steal].events[va.counts[va.index_to_steal]++] = ev;
			va.index_to_steal++;
			if (va.index_to_steal >= va.active_voice_count) va.index_to_steal = 0;
			break;
		}
		case note_off: {
			for (size_t i = 0; i < va.active_voice_count; ++i) {
				if (va.voices[i].midi_note == ev.midi_note) va.voices[i].events[va.counts[i]++] = ev;
			}
			break;
		}
		case pitch_wheel:
		case mod_wheel: {
			for (size_t i = 0; i < va.active_voice_count; ++i) { va.voices[i].events[va.counts[i]++] = ev; }
			break;
		}
		}
	}
}

inline void voice_process_event_for_frame(voice_state& v, size_t frame)
{
	const midi_event* best_event = nullptr;

	for (int i = 0; i < v.events.size(); i++) {
		const midi_event& ev = v.events[i];
		if (ev.offset == frame) {
			best_event = &ev;
			if (ev.type == note_on) break;
		}
	}

	if (best_event) switch (best_event->type) {
		case note_on:
			v.midi_note = best_event->midi_note;
			v.velocity = best_event->velocity;
			v.is_busy = true;
			v.gate = true;
			v.trigger = true;
			break;
		case note_off:
			v.gate = false;
			break;
		// TODO: handle pitch wheel and mod wheel events
		case pitch_wheel:
		case mod_wheel:
			break;
		}
}
} // namespace trnr

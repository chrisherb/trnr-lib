#pragma once

#include <array>
#include <cstddef>
#include <vector>

using namespace std;

namespace trnr {

enum midi_event_type {
	NOTE_ON = 0,
	NOTE_OFF,
	PITCH_WHEEL,
	MOD_WHEEL
};

struct midi_event {
	midi_event_type type;
	int offset;
	int midi_note;
	float velocity;
	double data;
};

inline void make_note_on(midi_event& ev, int _midi_note, float _velocity, int _offset = 0)
{
	ev.type = midi_event_type::NOTE_ON;
	ev.midi_note = _midi_note;
	ev.velocity = _velocity;
	ev.offset = _offset;
}

inline void make_note_off(midi_event& ev, int _midi_note, float _velocity, int _offset = 0)
{
	ev.type = midi_event_type::NOTE_OFF;
	ev.midi_note = _midi_note;
	ev.velocity = _velocity;
	ev.offset = _offset;
}

inline void make_pitch_wheel(midi_event& ev, double _pitch, int _offset = 0)
{
	ev.type = midi_event_type::PITCH_WHEEL;
	ev.data = _pitch;
}

inline void make_mod_wheel(midi_event& ev, double _mod, int _offset = 0)
{
	ev.type = midi_event_type::PITCH_WHEEL;
	ev.data = _mod;
}

#ifndef MAX_VOICES
#define MAX_VOICES 16
#endif

#ifndef MAX_EVENTS_PER_VOICE
#define MAX_EVENTS_PER_VOICE 32
#endif

struct voice_state {
	int midi_note;
	bool is_busy = false;
	bool gate = false;
	bool trigger = false;
	float velocity = 0.0f;
	array<midi_event, MAX_EVENTS_PER_VOICE> events;
	size_t event_count = 0;
};

struct voice_allocator {
	array<voice_state, MAX_VOICES> voices;
	int active_voice_count = 1;
	size_t index_to_steal = 0;
};

inline void voice_allocator_init(voice_allocator& va)
{
	for (size_t i = 0; i < MAX_VOICES; ++i) { va.voices[i].event_count = 0; }
}

inline void voice_allocator_process_block(voice_allocator& va, const vector<midi_event>& midi_events)
{
	// reset voice events and counts
	for (int i = 0; i < MAX_VOICES; i++) {
		for (int j = 0; j < MAX_EVENTS_PER_VOICE; j++) { va.voices[i].events[j] = midi_event {}; }
		va.voices[i].event_count = 0;
	}

	for (const auto& ev : midi_events) {
		switch (ev.type) {
		case NOTE_ON: {
			bool found = false;
			// attempt to find a free voice
			for (size_t i = 0; i < va.active_voice_count; ++i) {
				if (!va.voices[i].is_busy) {
					voice_state& found_voice = va.voices[i];
					found_voice.is_busy = true;
					found_voice.midi_note = ev.midi_note;
					found_voice.velocity = ev.velocity;
					found_voice.events[va.voices[i].event_count++] = ev;
					found = true;
					break;
				}
			}

			if (found) break;

			// if all voices are busy, steal one round-robin
			voice_state& found_voice = va.voices[va.index_to_steal];
			found_voice.is_busy = true;
			found_voice.midi_note = ev.midi_note;
			found_voice.velocity = ev.velocity;
			found_voice.events[va.voices[va.index_to_steal].event_count++] = ev;
			va.index_to_steal++;
			if (va.index_to_steal >= va.active_voice_count) va.index_to_steal = 0;
			break;
		}
		case NOTE_OFF: {
			for (size_t i = 0; i < va.active_voice_count; ++i) {
				if (va.voices[i].midi_note == ev.midi_note) va.voices[i].events[va.voices[i].event_count++] = ev;
			}
			break;
		}
		case PITCH_WHEEL:
		case MOD_WHEEL: {
			for (size_t i = 0; i < va.active_voice_count; ++i) { va.voices[i].events[va.voices[i].event_count++] = ev; }
			break;
		}
		}
	}
}

inline void voice_process_event_for_frame(voice_state& v, size_t frame)
{
	const midi_event* best_event = nullptr;

	for (int i = 0; i < v.event_count; i++) {
		const midi_event& ev = v.events[i];
		if (ev.offset == frame) {
			best_event = &ev;
			if (ev.type == NOTE_ON) break;
		}
	}

	if (best_event) switch (best_event->type) {
		case NOTE_ON:
			v.midi_note = best_event->midi_note;
			v.velocity = best_event->velocity;
			v.is_busy = true;
			v.gate = true;
			v.trigger = true;
			break;
		case NOTE_OFF:
			v.gate = false;
			break;
		// TODO: handle pitch wheel and mod wheel events
		case PITCH_WHEEL:
		case MOD_WHEEL:
			break;
		}
}
} // namespace trnr

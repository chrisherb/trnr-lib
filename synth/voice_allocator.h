#pragma once
#include "ivoice.h"
#include "midi_event.h"
#include <vector>

namespace trnr {

template <typename t_voice, typename t_sample>
class voice_allocator {
public:
	std::vector<t_voice*> voicePtrs;

	voice_allocator()
		: voicePtrs(4, new t_voice())
	{
		// checks whether template derives from ivoice
		typedef t_voice assert_at_compile_time[is_convertible<t_voice, t_sample>::value ? 1 : -1];

		voicePtrs.reserve(8);
	}

	void set_voice_count(const int& voice_count) { voicePtrs.resize(voice_count, voicePtrs.at(0)); }

	void note_on(const midi_event& event)
	{
		t_voice* voice = get_free_voice(event.midi_note);

		if (voice == nullptr) { voice = steal_voice(); }

		if (voice != nullptr) { voice->note_on(event.midi_note, event.velocity); }
	}

	void note_off(const midi_event& event)
	{
		for (t_voice* v : voicePtrs) {
			if (v->midi_note == event.midi_note) v->note_off();
		}
	}

	void access(std::function<void(t_voice*)> f) { std::for_each(voicePtrs.begin(), voicePtrs.end(), f); }

	void process_samples(t_sample** _outputs, int _start_index, int _block_size)
	{
		for (int b = _start_index; b < _start_index + _block_size; b += internal_block_size) {

			const int block_size = internal_block_size;

			// process all events in the block (introduces potential inaccuracy of up to 16 samples)
			process_events(b, block_size);

			for (t_voice* v : voicePtrs) { v->process_samples(_outputs, b, block_size); }
		}
	}

	void add_event(midi_event event) { input_queue.push_back(event); }

	bool voices_active()
	{
		bool voices_active = false;

		for (t_voice* v : voicePtrs) {
			bool busy = v->is_busy();
			voices_active |= busy;
		}

		return voices_active;
	}

	void set_samplerate(double _samplerate)
	{
		for (t_voice* v : voicePtrs) { v->set_samplerate(_samplerate); }
	}

private:
	std::vector<midi_event> input_queue;
	int index_to_steal = 0;
	const int internal_block_size = 16;

	t_voice* get_free_voice(float frequency)
	{
		t_voice* voice = nullptr;

		for (t_voice* v : voicePtrs) {
			if (!v->is_busy()) voice = v;
		}

		return voice;
	}

	t_voice* steal_voice()
	{
		t_voice* free_voice = nullptr;

		for (t_voice* v : voicePtrs) {
			if (!v->gate) {
				free_voice = v;
				break;
			}
		}

		if (free_voice == nullptr) {
			free_voice = voicePtrs.at(index_to_steal);

			if (index_to_steal < voicePtrs.size() - 1) {
				index_to_steal++;
			} else {
				index_to_steal = 0;
			}
		}

		return free_voice;
	}

	void process_events(int _start_index, int _block_size)
	{
		for (int s = _start_index; s < _start_index + _block_size; s++) {
			auto iterator = input_queue.begin();
			while (iterator != input_queue.end()) {

				midi_event& event = *iterator;
				if (event.offset == _start_index) {

					switch (event.type) {
					case midi_event_type::note_on:
						note_on(event);
						break;
					case midi_event_type::note_off:
						note_off(event);
						break;
					case midi_event_type::pitch_wheel:
						access([&event](t_voice* voice) { voice->modulate_pitch(event.data); });
						break;
					default:
						break;
					}

					iterator = input_queue.erase(iterator);
				} else {
					iterator++;
				}
			}
		}
	}
};
} // namespace trnr

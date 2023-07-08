#pragma once
#include <vector>
#include "note_event.h"

namespace trnr {

template <typename t_voice>
class voice_allocator {
public:
    std::vector<t_voice> voices;
    
    voice_allocator(const double& _samplerate)
        : samplerate { _samplerate }
        , voices(8, t_voice(_samplerate))
    {
    }

    void set_voice_count(const int& voice_count)
    {
        voices.resize(voice_count, voices.at(0));
    }

    void note_on(const note_event& event)
    {
        t_voice* voice = get_free_voice(event.midi_note);

        if (voice == nullptr) {
            voice = steal_voice();
        }

        if (voice != nullptr) {
            voice->note_on(event.midi_note, event.velocity);
        }
    }

    void note_off(const note_event& event)
    {
        for (auto it = voices.begin(); it != voices.end(); it++) {
            if ((*it).midi_note == event.midi_note) {
                (*it).note_off();
            }
        }
    }

    void access(std::function<void(t_voice&)> f)
    {
        std::for_each(voices.begin(), voices.end(), f);
    }

    void process_samples(double** _outputs, int _start_index, int _block_size)
    {
        for (int s = _start_index; s < _start_index + _block_size; s++) {

            process_events(s);

            float voices_signal = 0.;

            std::for_each(voices.begin(), voices.end(), [&voices_signal](t_voice& voice) {
                voices_signal += (voice.process_sample() / 3.);
            });

            _outputs[0][s] = voices_signal;
            _outputs[1][s] = voices_signal;
        }
    }

    void add_event(note_event event)
    {
        input_queue.push_back(event);
    }

    bool voices_active()
    {
        bool voices_active = false;

        for (auto it = voices.begin(); it != voices.end(); it++) {
            bool busy = (*it).is_busy();
            voices_active |= busy;
        }

        return voices_active;
    }

    void set_samplerate(double _samplerate)
    {
        this->samplerate = _samplerate;
        for (int i = 0; i < voices.size(); i++) {
            voices.at(i).set_samplerate(_samplerate);
        }
    }

private:
    double samplerate;
    std::vector<note_event> input_queue;

    t_voice* get_free_voice(float frequency)
    {
        t_voice* voice = nullptr;

        for (auto it = voices.begin(); it != voices.end(); it++) {
            if (!(*it).is_busy()) {
                voice = &*it;
            }
        }

        return voice;
    }

    t_voice* steal_voice()
    {
        t_voice* free_voice = nullptr;

        for (auto it = voices.begin(); it != voices.end(); it++) {
            if (!(*it).gate) {
                free_voice = &*it;
                break;
            }
        }

        if (free_voice == nullptr) {
            free_voice = &voices.at(0);
        }

        return free_voice;
    }

    void process_events(int _start_index)
    {
        auto iterator = input_queue.begin();
        while (iterator != input_queue.end()) {

            note_event& event = *iterator;
            if (event.offset == _start_index) {

                switch (event.type) {
                case note_event_type::note_on:
                    note_on(event);
                    break;
                case note_event_type::note_off:
                    note_off(event);
                    break;
                }

                iterator = input_queue.erase(iterator);
            } else {
                iterator++;
            }
        }
    }
};
}
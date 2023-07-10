#pragma once

namespace trnr {

enum midi_event_type {
    note_on = 0,
    note_off,
    pitch_wheel,
    mod_wheel
};

class midi_event {
public:
    midi_event_type type;
    int offset = 0;
    int midi_note = 0;
    float velocity = 1.f;
    double data = 0;

    void make_note_on(int _midi_note, float _velocity, int _offset = 0)
    {
        type = midi_event_type::note_on;
        midi_note = _midi_note;
        velocity = _velocity;
        offset = _offset;
    }

    void make_note_off(int _midi_note, float _velocity, int _offset = 0)
    {
        type = midi_event_type::note_off;
        midi_note = _midi_note;
        velocity = _velocity;
        offset = _offset;
    }

    void make_pitch_weel(double _pitch, int _offset = 0)
    {
        type = midi_event_type::pitch_wheel;
        data = _pitch;
    }

    void make_mod_weel(double _mod, int _offset = 0)
    {
        type = midi_event_type::pitch_wheel;
        data = _mod;
    }
};
}

#pragma once

namespace trnr::lib::synth {

enum note_event_type {
    note_on = 0,
    note_off
};

class note_event {
public:
    note_event_type type;
    int midi_note;
    float velocity;
    int offset;

    note_event(note_event_type type, int _midi_note, float _velocity, int _offset)
        : type { type }
        , midi_note { _midi_note }
        , velocity { _velocity }
        , offset { _offset }
    {
    }
};
}

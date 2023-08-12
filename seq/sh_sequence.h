#pragma once
#include "sequence.h"

namespace trnr {

struct note {
  note(bool _note_on = false, int _pitch = 30, int _velocity = 100) {
    note_on = _note_on;
    pitch = _pitch;
    velocity = _velocity;
  }

  bool note_on;
  int pitch;
  int velocity;
};

template <size_t t_length>
// sequence with chance control per step that samples incoming note information
class sh_sequence : public sequence<int, t_length> {
public:
  int m_root = 30;
  bool m_use_pitch;
  bool m_use_octave;
  bool m_use_velocity;

  std::vector<note> process_beat(int _pitch, int _octave, int _velocity, int _duration) {
    std::vector<note> notes;
    int chance = chance_sequence.process_beat();

    if (duration_left > 0) {
      duration_left--;
    }
    if (duration_left == 0) {
      note note_off = note(false, m_channel, m_sampled_pitch);
      notes.push_back(note_off);
      m_duration_left = -1; // reset
    }

    // 1 - 100
    int num = rand() % 100;

    if (num <= chance) {
      if (duration_left > 0) {
        // cut current note
        note note_off = note(false, m_channel, m_sampled_pitch);
        notes.push_back(note_off);
        m_duration_left = -1; // reset
      }

      duration_left = duration;
      note note_on = make_note(true, _pitch, _octave, _velocity);
      notes.push_back(note_on);
    }

    return notes;
  }

private:
  int m_duration_left = -1;
  int m_sampled_pitch = 0;
  sequence<int, t_length> chance_sequence;

  note make_note(bool note_on, int pitch, int octave, int velocity) {
    int calculated_pitch = m_root;
    int calculated_velocity = 100;

    if (m_use_octave) {
      calculated_pitch += octave * 12;
    }
    if (m_use_pitch) {
      calculated_pitch += pitch;
    }
    if (m_use_velocity) {
      calculated_velocity = velocity;
    }

    m_sampled_pitch = calculated_pitch;

    return note(note_on, calculated_pitch, calculated_velocity);
  }
};
} // namespace trnr
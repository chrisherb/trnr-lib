#pragma once

#include "simple_seq.h"

namespace trnr {

struct combine_note {
	int probability;
	int pitch;
	int octave;
	int velocity;
};

struct combine_seq {
	simple_seq probability_seq;
	simple_seq pitch_seq;
	simple_seq octave_seq;
	simple_seq velocity_seq;
	combine_note current_note;
};

inline void combine_seq_init(combine_seq& c, size_t max_len)
{
	simple_seq_init(c.probability_seq, max_len);
	simple_seq_init(c.pitch_seq, max_len);
	simple_seq_init(c.octave_seq, max_len);
	simple_seq_init(c.velocity_seq, max_len);
}

inline combine_note& combine_seq_process_step(combine_seq& c)
{
	c.current_note.probability = simple_seq_process_step(c.probability_seq);
	c.current_note.pitch = simple_seq_process_step(c.pitch_seq);
	c.current_note.octave = simple_seq_process_step(c.octave_seq);
	c.current_note.velocity = simple_seq_process_step(c.velocity_seq);

	return c.current_note;
}

struct timing_info {};
} // namespace trnr

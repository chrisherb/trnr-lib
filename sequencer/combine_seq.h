/*
 * combine_seq.h
 * Copyright (c) 2025 Christopher Herb
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

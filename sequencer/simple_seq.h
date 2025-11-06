/*
 * simple_seq.h
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

#include <cstddef>
#include <cstdlib>
#include <vector>

using namespace std;

namespace trnr {

enum playback_dir {
	PB_FORWARD,
	PB_BACKWARD,
	PB_PENDULUM,
	PB_RANDOM
};

struct simple_seq {
	size_t current_pos;
	size_t length;
	playback_dir playback_dir;
	vector<int> data;
	bool pendulum_forward;
};

inline void simple_seq_init(simple_seq& s, size_t length)
{
	s.current_pos = 0;
	s.length = length;
	s.playback_dir = PB_FORWARD;
	s.data.resize(s.length);
	s.data.assign(s.length, 0);
	s.pendulum_forward = true;
}

inline int simple_seq_process_step(simple_seq& s)
{
	// play head advancement
	if (s.playback_dir == PB_FORWARD ||
		s.playback_dir == PB_PENDULUM && s.pendulum_forward)
		s.current_pos++;
	else if (s.playback_dir == PB_BACKWARD ||
			 s.playback_dir == PB_PENDULUM && !s.pendulum_forward)
		s.current_pos--;
	else if (s.playback_dir == PB_RANDOM) s.current_pos = rand() % s.length;

	// play head reset
	switch (s.playback_dir) {
	case PB_FORWARD:
		if (s.current_pos > s.length - 1) s.current_pos = 0;
		break;
	case PB_BACKWARD:
		if (s.current_pos < 0) s.current_pos = s.length - 1;
		break;
	case PB_PENDULUM:
		if (s.pendulum_forward && s.current_pos >= s.length - 1)
			s.pendulum_forward = false;
		else if (!s.pendulum_forward && s.current_pos <= 0) s.pendulum_forward = true;
		break;
	case PB_RANDOM:
		break;
	}

	return s.data[s.current_pos];
}
} // namespace trnr

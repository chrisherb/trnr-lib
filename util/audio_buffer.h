#pragma once

#include <vector>

using namespace std;

namespace trnr {
template <typename t_sample>
struct audio_buffer {
	size_t channels;
	size_t frames;

	vector<t_sample> flat_data;
	vector<t_sample*> channel_ptrs;
};

template <typename t_sample>
void audio_buffer_init(audio_buffer<t_sample>& a, size_t channels, size_t frames)
{
	a.channels = channels;
	a.frames = frames;
	a.flat_data.resize(channels * frames);
	a.channel_ptrs.resize(channels);
	audio_buffer_update_ptrs(a);
}

template <typename t_sample>
void audio_buffer_update_ptrs(audio_buffer<t_sample>& a)
{
	for (int ch = 0; ch < a.channels; ++ch) { a.channel_ptrs[ch] = a.flat_data.data() + ch * a.frames; }
}
} // namespace trnr

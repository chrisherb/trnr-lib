#pragma once
#include <algorithm>
#include <cassert>
#include <vector>

template <typename t_sample>
class audio_buffer {
public:
	audio_buffer(const t_sample** input, int channels, int frames)
		: m_data(channels * frames)
		, m_channels(channels)
		, m_frames(frames)
		, m_channel_ptrs(channels)
	{
		for (int ch = 0; ch < channels; ++ch) {
			std::copy(input[ch], input[ch] + frames, m_data.begin() + ch * frames);
		}
		update_channel_ptrs();
	}

	audio_buffer(int channels, int frames)
		: m_channels(channels)
		, m_frames(frames)
		, m_data(channels * frames)
		, m_channel_ptrs(channels)
	{
		update_channel_ptrs();
	}

	void set_size(int channels, int frames)
	{
		m_channels = channels;
		m_frames = frames;
		m_data.resize(channels * frames);
		m_channel_ptrs.resize(channels);
		update_channel_ptrs();
	}

	void set_data(const t_sample** input, int channels, int frames)
	{
		set_size(channels, frames);
		for (int ch = 0; ch < channels; ++ch) {
			std::copy(input[ch], input[ch] + frames, m_data.begin() + ch * frames);
		}
		update_channel_ptrs();
	}

	int num_samples() const { return m_frames; }

	int num_channels() const { return m_channels; }

	t_sample* data() { return m_data.data(); }

	const t_sample* data() const { return m_data.data(); }

	// t_sample** access, always up-to-date after construction/resize
	t_sample** write_ptrs() { return m_channel_ptrs.data(); }

	t_sample* write_ptr(int channel)
	{
		assert(channel >= 0 && channel < m_channels);
		return m_channel_ptrs[channel];
	}

	const t_sample* const* channel_ptrs() const { return m_channel_ptrs.data(); }

private:
	void update_channel_ptrs()
	{
		for (int ch = 0; ch < m_channels; ++ch) { m_channel_ptrs[ch] = m_data.data() + ch * m_frames; }
	}

	std::vector<t_sample> m_data;
	int m_channels;
	int m_frames; // samples per channel
	std::vector<t_sample*> m_channel_ptrs;
};
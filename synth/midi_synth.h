#pragma once
#include <memory>
#include <vector>
#include "note_event.h"
#include "voice_allocator.h"

namespace trnr {

// a generic midi synth base class with sample accurate event handling.
template <typename t_voice>
class midi_synth : public voice_allocator<t_voice> {
public:
    midi_synth(int _n_voices, double _samplerate, int _block_size)
        : voice_allocator<t_voice> { _samplerate }
        , m_samplerate { _samplerate }
        , m_block_size { _block_size }
        , m_voices_active { false }
    {
    }

    void set_samplerate(double _samplerate, int _block_size)
    {
        m_samplerate = _samplerate;
        m_block_size = _block_size;
        voice_allocator::set_samplerate(_samplerate);
    }

    void process_block(double** _outputs, int _n_frames)
    {
        // sample accurate event handling based on the iPlug2 synth by Oli Larkin
        if (m_voices_active || !m_event_queue.empty()) {
            int block_size = m_block_size;
            int samples_remaining = _n_frames;
            int start_index = 0;

            while (samples_remaining > 0) {

                if (samples_remaining < block_size)
                    block_size = samples_remaining;

                while (!m_event_queue.empty()) {
                    note_event event = m_event_queue.front();

                    // we assume the messages are in chronological order. If we find one later than the current block we are done.
                    if (event.offset > start_index + block_size)
                        break;

                    // send performance messages to the voice allocator
                    // message offset is relative to the start of this process_samples() block
                    event.offset -= start_index;
                    voice_allocator::add_event(event);

                    m_event_queue.erase(m_event_queue.begin());
                }

                voice_allocator::process_samples(_outputs, start_index, block_size);

                samples_remaining -= block_size;
                start_index += block_size;
            }

            m_voices_active = voice_allocator::voices_active();

            flush_event_queue(_n_frames);
        } else {
            for (int s = 0; s < _n_frames; s++) {
                _outputs[0][s] = 0.;
                _outputs[1][s] = 0.;
            }
        }
    }
    
    void add_event(note_event event)
    {
        if (event.type == note_event_type::note_on)
            m_voices_active = true;

        m_event_queue.push_back(event);
    }

    void flush_event_queue(int frames)
    {
        for (int i = 0; i < m_event_queue.size(); i++) {
            m_event_queue.at(i).offset -= frames;
        }
    }

private:
    std::vector<note_event> m_event_queue;
    double m_samplerate;
    int m_block_size;
    bool m_voices_active;
};
}

#pragma once
#include "ivoice.h"
#include "midi_event.h"
#include "voice_allocator.h"
#include <memory>
#include <vector>

namespace trnr {

// a generic midi synth base class with sample accurate event handling.
// the templated type t_voice must derive from ivoice
template <typename t_voice>
class midi_synth : public voice_allocator<t_voice> {
public:
    midi_synth(int _n_voices)
        : m_voices_active { false }
    {
        // checks whether template derives from ivoice
        typedef t_voice assert_at_compile_time[is_convertible<t_voice>::value ? 1 : -1];
    }

    void set_samplerate_blocksize(double _samplerate, int _block_size)
    {
        m_block_size = _block_size;
        voice_allocator<t_voice>::set_samplerate(_samplerate);
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
                    midi_event event = m_event_queue.front();

                    // we assume the messages are in chronological order. If we find one later than the current block we are done.
                    if (event.offset > start_index + block_size)
                        break;

                    // send performance messages to the voice allocator
                    // message offset is relative to the start of this process_samples() block
                    event.offset -= start_index;
                    voice_allocator<t_voice>::add_event(event);

                    m_event_queue.erase(m_event_queue.begin());
                }

                voice_allocator<t_voice>::process_samples(_outputs, start_index, block_size);

                samples_remaining -= block_size;
                start_index += block_size;
            }

            m_voices_active = voice_allocator<t_voice>::voices_active();

            flush_event_queue(_n_frames);
        } else {
            for (int s = 0; s < _n_frames; s++) {
                _outputs[0][s] = 0.;
                _outputs[1][s] = 0.;
            }
        }
    }

    void add_event(midi_event event)
    {
        if (event.type == midi_event_type::note_on)
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
    std::vector<midi_event> m_event_queue;
    int m_block_size;
    bool m_voices_active;
};
}

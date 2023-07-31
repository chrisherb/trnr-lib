#pragma once

#include "../filter/chebyshev.h"
#include "../companding/ulaw.h"
#include <iostream>

namespace trnr {

struct retro_buf_modulation {
    double midi_note;
    double pitch_mod;
    double samplerate; // the (re)samplerate
    double bitrate;
    size_t start; // sets the start point from which to play
    size_t end; // sets the end point
    bool looping; // sets whether the sample should loop
    bool reset; // resets the phase
    int jitter; // jitter amount
    double deviation;
};

// base class for accessing a sample buffer with adjustable samplerate, bitrate and other options.
class retro_buf {
public:
    void set_host_samplerate(double _samplerate) {
        m_host_samplerate = _samplerate;
        m_imaging_filter_l.set_samplerate(_samplerate);
        m_imaging_filter_r.set_samplerate(_samplerate);
    }

    void set_buf_samplerate(double _samplerate) {
        m_buf_samplerate = _samplerate;
    }

    void set_buffer_size(size_t _buffer_size) {
        m_buffer_size = _buffer_size;
    }

    void set_channel_count(size_t _channel_count) {
        m_channel_count = _channel_count;
    }

    void start_playback() {
        if (m_modulation.reset || (!m_modulation.reset && m_playback_pos == -1)) {
            m_playback_pos = (double)m_modulation.start;
        }
    }

    void process_block(double** _outputs, size_t _block_size, retro_buf_modulation _mod) {

        m_modulation = _mod;

        for (int i = 0; i < _block_size; ++i) {
            double output_l = 0;
            double output_r = 0;
            
            // if within bounds
            if (m_playback_pos > -1 && m_playback_pos <= _mod.end)  {
                
                // quantize index
                double samplerate_divisor = m_host_samplerate / _mod.samplerate;
                size_t quantized_index = static_cast<size_t>(static_cast<size_t>(m_playback_pos / samplerate_divisor) * samplerate_divisor);

                // get sample for each channel
                output_l = get_sample((size_t)quantized_index, 0);
                if (m_channel_count > 0) {
                    output_r = get_sample(wrap(quantized_index + calc_jitter(_mod.jitter), m_buffer_size), 1);
                } else {
                    output_r = output_l;
                }
                
                // advance position
                double note_ratio = midi_to_ratio(_mod.midi_note + _mod.pitch_mod);
                m_playback_pos += note_ratio * (m_buf_samplerate / m_host_samplerate);
            
                reduce_bitrate(output_l, output_r, _mod.bitrate);

                // calculate imaging filter frequency + deviation
                double filter_frequency = ((_mod.samplerate / 2) * note_ratio) * _mod.deviation;

                m_imaging_filter_l.process_sample(output_l, filter_frequency);
                m_imaging_filter_r.process_sample(output_r, filter_frequency);
            }
            // else if loop
            else if(_mod.looping) {
                // loop
                m_playback_pos = (double)_mod.start;
            }
            // else
            else {
                // stop
                m_playback_pos = -1;
            }

            _outputs[0][i] = output_l;
            _outputs[1][i] = output_r;
        }
    }

    virtual float get_sample(size_t _index, size_t _channel) = 0;

private:
    size_t m_channel_count = 0;
    size_t m_buffer_size = 0;
    double m_buf_samplerate = 44100.0;
    double m_host_samplerate = 44100.0;
    double m_playback_pos = -1;

    chebyshev m_imaging_filter_l;
    chebyshev m_imaging_filter_r;
    ulaw m_compander;
    retro_buf_modulation m_modulation;

    float midi_to_ratio(double midi_note) {
        return powf(powf(2, (float)midi_note - 60.f), 1.f / 12.f);
    }
    
    template <typename T>
    T clamp(T& value, T min, T max) {
        if (value < min) {
            value = min;
        } else if (value > max) {
            value = max;
        }
        return value;
    }

    double wrap(double value, double max) {
        while (value > max) {
            value =- max;
        }
        return value;
    }

    int calc_jitter(int jitter) {
        if (jitter > 0) {
            return static_cast<int>(rand() % jitter);
        } else {
            return 0;
        }
    }

    void reduce_bitrate(double& value1, double& value2, double bit) {
        m_compander.encode_samples(value1, value2);

        float resolution = powf(2, bit);
        value1 = round(value1 * resolution) / resolution;
        value2 = round(value2 * resolution) / resolution;

        m_compander.decode_samples(value1, value2);
    }
};
}
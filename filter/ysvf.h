#pragma once
#include "ylowpass.h"
#include "yhighpass.h"
#include "ybandpass.h"
#include "ynotch.h"

namespace trnr {
template <typename t_sample>

enum filter_types {
    lowpass = 0,
    highpass,
    bandpass,
    notch
};

class ysvf {
public:
    ysvf(double _samplerate)
        : lowpass { _samplerate }
        , highpass { _samplerate }
        , bandpass { _samplerate }
        , notch { _samplerate }
    {}

    void set_samplerate(double _samplerate) {
        lowpass.set_samplerate(_samplerate);
        highpass.set_samplerate(_samplerate);
        bandpass.set_samplerate(_samplerate);
        notch.set_samplerate(_samplerate);
    }

    void set_filter_type(filter_types type) {
        filter_type = type;
    }

    void set_drive(float value) {
        lowpass.set_drive(value);
        highpass.set_drive(value);
        bandpass.set_drive(value);
        notch.set_drive(value);
    }

    void set_frequency(float value) {
        lowpass.set_frequency(value);
        highpass.set_frequency(value);
        bandpass.set_frequency(value);
        notch.set_frequency(value);
    }

    void set_resonance(float value) {
        lowpass.set_resonance(value);
        highpass.set_resonance(value);
        bandpass.set_resonance(value);
        notch.set_resonance(value);
    }

    void set_edge(float value) {
        lowpass.set_edge(value);
        highpass.set_edge(value);
        bandpass.set_edge(value);
        notch.set_edge(value);
    }

    void set_output(float value) {
        lowpass.set_output(value);
        highpass.set_output(value);
        bandpass.set_output(value);
        notch.set_output(value);
    }

    void set_mix(float value) {
        lowpass.set_mix(value);
        highpass.set_mix(value);
        bandpass.set_mix(value);
        notch.set_mix(value);
    }

    void process_block(t_sample** inputs, t_sample** outputs, int block_size) {

        switch (filter_type) {
            case filter_types::lowpass:
                lowpass.processblock(inputs, outputs, block_size);
                break;
            case filter_types::highpass:
                highpass.processblock(inputs, outputs, block_size);
                break;
            case filter_types::bandpass:
                bandpass.processblock(inputs, outputs, block_size);
                break;
            case filter_types::notch:
                notch.processblock(inputs, outputs, block_size);
                break;
        }
    }

private:
    filter_types filter_type;
    ylowpass lowpass;
    yhighpass highpass;
    ybandpass bandpass;
    ynotch notch;

    double clamp(double& value, double min, double max) {
        if (value < min) {
            value = min;
        } else if (value > max) {
            value = max;
        }
        return value;
    }
};
}
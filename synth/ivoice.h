#pragma once

namespace trnr {
struct ivoice {
    virtual ~ivoice() = default;
    virtual float process_sample() = 0;
    virtual bool is_busy() = 0;
    virtual void set_samplerate(double samplerate) = 0;
    virtual void note_on(int _note, float _velocity) = 0;
    virtual void note_off() = 0;
    virtual void modulate_pitch(float _pitch) = 0;
};

// check if a template derives from ivoice
template <class derived>
struct is_convertible {
    template <class T>
    static char test(T*);

    template <class T>
    static double test(...);

    static const bool value = sizeof(test<ivoice>(static_cast<derived*>(0))) == 1;
};
}
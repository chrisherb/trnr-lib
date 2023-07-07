#pragma once
#include <cstdint>

namespace trnr::lib::companding {
// mulaw companding based on code by Emilie Gillet / Mutable Instruments
class mulaw {
public:
    int8_t encode_samples(int16_t pcm_val) {
        int16_t mask;
        int16_t seg;
        uint8_t uval;
        pcm_val = pcm_val >> 2;
        if (pcm_val < 0) {
            pcm_val = -pcm_val;
            mask = 0x7f;
        } else {
            mask = 0xff;
        }
        if (pcm_val > 8159) pcm_val = 8159;
        pcm_val += (0x84 >> 2);
        
        if (pcm_val <= 0x3f) seg = 0;
        else if (pcm_val <= 0x7f) seg = 1;
        else if (pcm_val <= 0xff) seg = 2;
        else if (pcm_val <= 0x1ff) seg = 3;
        else if (pcm_val <= 0x3ff) seg = 4;
        else if (pcm_val <= 0x7ff) seg = 5;
        else if (pcm_val <= 0xfff) seg = 6;
        else if (pcm_val <= 0x1fff) seg = 7;
        else seg = 8;
        if (seg >= 8)
            return static_cast<uint8_t>(0x7f ^ mask);
        else {
            uval = static_cast<uint8_t>((seg << 4) | ((pcm_val >> (seg + 1)) & 0x0f));
            return (uval ^ mask);
        }
    }

    int16_t decode_samples(uint8_t u_val) {
        int16_t t;
        u_val = ~u_val;
        t = ((u_val & 0xf) << 3) + 0x84;
        t <<= ((unsigned)u_val & 0x70) >> 4;
        return ((u_val & 0x80) ? (0x84 - t) : (t - 0x84));
    }
};
}
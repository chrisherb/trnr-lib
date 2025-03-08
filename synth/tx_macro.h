#include <bitset>

const size_t NUM_PARAMS = 128;

// Define the bitmask flags for different controls
enum ControlFlags {
    OP1_ENVELOPE_ATTACK1 = 0,
    
    NUM_FLAGS = 128
};

namespace trnr {
    struct tx_macro {
        float range_min;
        float range_max;
        float exponent;
        std::bitset<NUM_PARAMS> control_flags; // bitset flags indicating which parameters this control affects

        tx_macro(float _range_min, float _range_max, float _exponent, const std::bitset<NUM_PARAMS>& _flags)
            : range_min(_range_min), range_max(_range_max), exponent(_exponent), control_flags(_flags) {}
    }
}
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace trnr {
class sample {
public:
    sample(int16_t initial_value = 0) {
        set_value(initial_value);
        instances.push_back(this); // track this instance
    }

    ~sample() {
        // remove this instance from the tracking vector
        auto it = std::find(instances.begin(), instances.end(), this);
        if (it != instances.end()) {
            instances.erase(it);
        }
    }

    // set value while ensuring the global bit depth
    void set_value(int16_t new_value) {
        int16_t max_val = (1 << (global_bit_depth - 1)) - 1;  // Max value for signed bit depth
        int16_t min_val = -(1 << (global_bit_depth - 1));     // Min value for signed bit depth

        // Clamp the value to the allowed range
        if (new_value > max_val) {
            value = max_val;
        } else if (new_value < min_val) {
            value = min_val;
        } else {
            value = new_value;
        }
    }

    int16_t get_value() const {
        return value;
    }

    static void set_global_bit_depth(int depth) {
        if (depth < 1 || depth > 16) {
            throw std::invalid_argument("Bit depth must be between 1 and 16.");
        }

        // rescale all existing values if the bit depth changes
        float scaling_factor = get_scaling_factor(previous_bit_depth, global_bit_depth);

        // Rescale all existing instances
        for (auto* instance : instances) {
            instance->value = std::round(instance->value * scaling_factor);
        }

        previous_bit_depth = global_bit_depth; // Store the old bit depth
        global_bit_depth = depth; // Update the global bit depth
    }
    
    static int get_global_bit_depth() {
        return global_bit_depth;
    }

    // arithmetic operators (all respect global bit depth)
    sample operator+(const sample& other) const {
        return sample(this->value + other.value);
    }

    sample operator-(const sample& other) const {
        return sample(this->value - other.value);
    }

    sample operator*(const sample& other) const {
        return sample(this->value * other.value);
    }

    sample operator/(const sample& other) const {
        if (other.value == 0) {
            throw std::runtime_error("division by zero.");
        }
        return sample(this->value / other.value);
    }

    // Cast to int16_t for convenience
    operator int16_t() const {
        return value;
    }

private:
    int16_t value;
    static int global_bit_depth; // global bit depth
    static int previous_bit_depth; // previous bit depth
    static std::vector<sample*> instances; // track all instances for rescaling

    // helper function to calculate the scaling factor
    static float get_scaling_factor(int old_bit_depth, int new_bit_depth) {
        if (old_bit_depth == new_bit_depth) {
            return 1.0f; // No scaling needed
        }
        float old_max = (1 << (old_bit_depth - 1)) - 1;
        float new_max = (1 << (new_bit_depth - 1)) - 1;
        return new_max / old_max;
    }
};
}
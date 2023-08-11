#pragma once

namespace trnr {

template <typename t_type, size_t t_max_length>
// step sequence with arbitrary length and data
class sequence {
public:
    std::array<t_type, length> m_data;

    t_type process_beat() {
        t_type value = m_data[m_position];

        m_position++;

        if (m_position >= m_length) {
            m_position = 0;
        }

        return value;
    }

private:
    int m_position = 0;
    int m_length = t_max_length;
};
}
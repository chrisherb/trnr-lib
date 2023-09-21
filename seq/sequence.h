#pragma once

namespace trnr {

template <typename t_type, size_t t_max_length>
// step sequence with arbitrary length and data
class sequence {

	enum direction {
		forward = 0,
		backward,
		pendulum
	};

public:
	void set_data_point(size_t _index, t_type _data) { m_data[_index] = _data; }

	void set_direction(direction _direction) { m_direction = _direction; }

	void set_clock_division(int _division) { m_clock_division = _division; }

	void set_length(int length) { m_length = length; }

	t_type process_beat()
	{
		t_type value = m_data[m_position];

		switch (m_direction) {
		case forward:
			advance_position(true);
			// reset position
			if (m_position >= m_length) { m_position = 0; }
			break;
		case backward:
			advance_position(false);
			break;
		case pendulum:
			break;
		}

		// calculate clock division
		m_position_count++;
		if (m_position_count >= m_position) { m_position_count = 0; }

		return value;
	}

private:
	std::array<t_type, t_max_length> m_data;
	direction m_direction = forward;
	int m_position = 0;
	int m_position_count = 0;
	int m_clock_division = 1;
	int m_length = t_max_length;

	void advance_position(bool _forward)
	{
		int increment = _forward ? 1 : -1;
		if (m_position_count == 0) {
			if (_forward) m_position++;
			else m_position--;
		}
	}
};
} // namespace trnr

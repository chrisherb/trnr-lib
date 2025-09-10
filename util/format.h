#pragma once
#include <cstdarg>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace trnr {

// Formats a string (like printf) using a format string and variable arguments.
inline std::string format(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	std::vector<char> buf(256);
	int needed = vsnprintf(buf.data(), buf.size(), fmt, args);
	va_end(args);

	if (needed < 0) { return {}; }
	if (static_cast<size_t>(needed) < buf.size()) { return std::string(buf.data(), needed); }

	// Resize and try again if the buffer was too small
	buf.resize(needed + 1);
	va_start(args, fmt);
	vsnprintf(buf.data(), buf.size(), fmt, args);
	va_end(args);

	return std::string(buf.data(), needed);
}

inline std::string float_to_string_trimmed(float value)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(2) << value;
	std::string str = out.str();

	// Remove trailing zeros
	str.erase(str.find_last_not_of('0') + 1, std::string::npos);
	// If the last character is a decimal point, remove it as well
	if (!str.empty() && str.back() == '.') { str.pop_back(); }
	return str;
}
} // namespace trnr
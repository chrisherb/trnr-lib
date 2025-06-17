#pragma once
#include <cstdarg>
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
} // namespace trnr
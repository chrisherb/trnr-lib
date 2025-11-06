/*
 * format.h
 * Copyright (c) 2025 Christopher Herb
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <algorithm>
#include <cstdarg>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace trnr {

// Formats a string (like printf) using a format string and variable arguments.
inline string format(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vector<char> buf(256);
	int needed = vsnprintf(buf.data(), buf.size(), fmt, args);
	va_end(args);

	if (needed < 0) { return {}; }
	if (static_cast<size_t>(needed) < buf.size()) { return string(buf.data(), needed); }

	// Resize and try again if the buffer was too small
	buf.resize(needed + 1);
	va_start(args, fmt);
	vsnprintf(buf.data(), buf.size(), fmt, args);
	va_end(args);

	return string(buf.data(), needed);
}

inline string float_to_string_trimmed(float value)
{
	ostringstream out;
	out << fixed << setprecision(2) << value;
	string str = out.str();

	// Remove trailing zeros
	str.erase(str.find_last_not_of('0') + 1, string::npos);
	// If the last character is a decimal point, remove it as well
	if (!str.empty() && str.back() == '.') { str.pop_back(); }
	return str;
}

inline string to_upper(string& str)
{
	std::transform(str.begin(), str.end(), str.begin(),
				   [](unsigned char c) { return std::toupper(c); });
	return str;
}
} // namespace trnr

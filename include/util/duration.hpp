#pragma once

#include <string>
#include <sstream>
#include <iomanip>

namespace util {

// Simple POD to represent minutes and seconds
struct MinutesSeconds {
	int minutes;
	int seconds; // 0..59
};

// Split total seconds into minutes and seconds (seconds in [0,59]).
// Negative values are handled by applying the sign to minutes.
inline MinutesSeconds split_mm_ss(int total_seconds) {
	int sign = (total_seconds < 0) ? -1 : 1;
	int abs_s = total_seconds * sign;
	MinutesSeconds ms{
		/*minutes*/ sign * (abs_s / 60),
		/*seconds*/ abs_s % 60
	};
	return ms;
}

// Format seconds as M:SS (e.g., 3:05, 120 -> 2:00). Negative durations are prefixed with '-'.
inline std::string format_mm_ss(int total_seconds) {
	bool neg = total_seconds < 0;
	int abs_s = neg ? -total_seconds : total_seconds;
	int minutes = abs_s / 60;
	int seconds = abs_s % 60;

	std::ostringstream oss;
	if (neg) oss << '-';
	oss << minutes << ':' << std::setw(2) << std::setfill('0') << seconds;
	return oss.str();
}

} // namespace util


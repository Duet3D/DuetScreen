/*
 * utils.h
 *
 *  Created on: 26 Jan 2024
 *      Author: Andy Everitt
 */

#ifndef JNI_LOGIC_UTILS_H_
#define JNI_LOGIC_UTILS_H_

#include <cstdarg>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

#define UNUSED(x) (void)(x)

template <class... Ts>
struct overloaded : Ts...
{
	using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace utils
{
	template <typename T>
	T bound(T value, T min, T max)
	{
		return value < min ? min : (value > max ? max : value);
	}
	std::string exec(const char* cmd);
	size_t removeCharFromString(std::string& nString, char c);
	void replaceSubstring(std::string& nString, const std::string& target, const std::string& replacement);
	std::vector<std::string> splitString(const std::string& str, const std::string& delimiter);
	size_t findInstance(std::string_view str, std::string_view target, size_t instance);
	/** `value` with at most `maxDecimals` decimals and no trailing zeros in them, e.g. 2.50 is "2.5" and 100 "100" */
	std::string formatFloat(double value, int maxDecimals = 6);

	std::string readFileToString(std::filesystem::path file);
} // namespace utils

#endif /* JNI_LOGIC_UTILS_H_ */

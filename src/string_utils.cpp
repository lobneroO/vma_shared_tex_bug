
#include "string_utils.h"

#include <algorithm>

namespace Stringutils
{

std::vector<std::string> split(std::string inStr, const std::string& delimeter)
{
	std::vector<std::string> outVec;
	size_t pos = 0;
	std::string token;
	while ((pos = inStr.find(delimeter)) != std::string::npos)
	{
		token = inStr.substr(0, pos);
		outVec.push_back(token);
		inStr.erase(0, pos + delimeter.length());
	}

	// the loop will miss the last substring, but due to the erase
	// inStr will consist only of the last substring. therefore add
	// it to the return vector
	outVec.push_back(inStr);
	return outVec;
}

std::string toLower(const std::string& inputStr)
{
	std::string cpy = inputStr;
	std::transform(cpy.begin(), cpy.end(), cpy.begin(),
		[](unsigned char c) { return std::tolower(c); });

	return cpy;
}

bool contains(const std::string& inputStr, const std::string& subStr, const bool& caseSensitive /* = true; */)
{
	// TODO: in C++23, std::string will ship with a contains method. replace it when upgrading
	// https://en.cppreference.com/w/cpp/string/basic_string/contains

	const std::string in = caseSensitive ? inputStr :  toLower(inputStr);
	const std::string sub = caseSensitive ? subStr : toLower(subStr);
	
	return in.find(sub) != std::string::npos;
}
}

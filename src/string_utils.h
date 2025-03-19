#pragma once

#include <string>
#include <vector>

namespace StringUtils
{
/**
* Splits the string at the given delimeter.
* The delimeter itself will be removed from the substring(s)
* @param inStr The string that is to be split
* @param delimeter The character(s) at which to split
* @returns The split string as a vector of strings
*/
std::vector<std::string> split(std::string inStr, const std::string& delimeter);

/**
* Converts the input string to the same string but with all previously upper case characters
* Now being lower case
* @param inputStr The input string
* @returns The same string but with all lower case characters
*/
std::string toLower(const std::string& inputStr);

/**
* @param inputStr The string that may or may not contain a substring
* @param subStr The substring to search for in the input string
* @param caseSensitive Whether to be case sensitive in the search for the substring. By default is true
* @returns whether the input string contains the substring
*/
bool contains(const std::string& inputStr, const std::string& subStr, const bool& caseSensitive = true);

}

#ifndef __util_h__
#define __util_h__

#include <string>
#include <vector>

std::string makeHash(const std::string& input);
std::string to_string(int number);
std::string randomString(const int len);
bool doubleOverflow(const double& x, const double& y);
unsigned int split(const std::string &txt, std::vector<std::string> &strs, char ch);

#endif
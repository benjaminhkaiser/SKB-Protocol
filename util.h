#ifndef __util_h__
#define __util_h__

#include <string>

std::string makeHash(const std::string& input);
std::string to_string(int number);
std::string randomString(const int len);
bool doubleOverflow(const double& x, const double& y);

#endif
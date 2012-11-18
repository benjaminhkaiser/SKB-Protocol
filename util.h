#ifndef __util_h__
#define __util_h__

#include <string>
#include <vector>
#include <stdlib.h>
std::string makeHash(const std::string& input);
std::string to_string(int number);
std::string to_string(double number);
std::string randomString(const int len);
long double string_to_Double(const std::string& input);
bool doubleOverflow(const double& x, const double& y);
int split(const std::string &s, char delim, std::vector<std::string> &elems);
void buildPacket(char* packet, std::string command);
bool sendPacket(long int &csock, void* packet);
bool listenPacket(long int &csock, char* packet);
bool isDouble(std::string questionable_string);
void encryptPacket(void* packet);
void decryptPacket(void* packet);
#endif

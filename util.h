#ifndef __util_h__
#define __util_h__

#include <string>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "includes/cryptopp/osrng.h"
#include "includes/cryptopp/cryptlib.h"
#include "includes/cryptopp/filters.h"
#include "includes/cryptopp/aes.h"
#include "includes/cryptopp/hex.h"
#include "includes/cryptopp/ccm.h"



std::string makeHash(const std::string& input);
std::string to_string(int number);
std::string to_string(double number);
std::string randomString(const unsigned int len);
long double string_to_Double(const std::string& input);
bool doubleOverflow(const long double& x, const long double& y);
int split(const std::string &s, char delim, std::vector<std::string> &elems);
void buildPacket(char* packet, std::string command);
bool sendPacket(long int &csock, void* packet);
bool listenPacket(long int &csock, char* packet);
bool isDouble(std::string questionable_string);
bool encryptPacket(char* packet, byte* aes_key);
bool decryptPacket(char* packet, byte* aes_key);
void generateRandomKey(std::string name, byte* key, long unsigned int length);
#endif

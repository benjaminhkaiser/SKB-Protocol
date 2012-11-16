#include <util.h>
#include <string>
#include <sstream>
#include <limits>
#include <algorithm>
#include <vector>
#include "includes/cryptopp/sha.h"
#include "includes/cryptopp/hex.h"


long double string_to_Double(const std::string& input_string)
{
	return strtold(input_string.c_str(), NULL);
} //end string_to_Double function

std::string makeHash(const std::string& input)
{
	CryptoPP::SHA512 hash;
	byte digest[ CryptoPP::SHA512::DIGESTSIZE ];

	hash.CalculateDigest( digest, (byte*) input.c_str(), input.length() );

	CryptoPP::HexEncoder encoder;
	std::string output;
	encoder.Attach( new CryptoPP::StringSink( output ) );
	encoder.Put( digest, sizeof(digest) );
	encoder.MessageEnd();

	return output;
}

std::string to_string(int number)
{
   std::stringstream ss;
   ss << number;
   return ss.str();
}

std::string to_string(double number)
{
   std::stringstream ss;
   ss << number;
   return ss.str();
}

std::string randomString(const int len)
{
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string s = "";
    for (int i = 0; i < len; ++i) {
        s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return s;
}
//Shouldn't we only need to check if it is between 0 and max?
bool doubleOverflow(const double& x, const double& y)
{
	double max = std::numeric_limits<double>::max();
	double min = std::numeric_limits<double>::min();

	if(y > 0 && x > 0)
	{
		if(y <= max - x)
		{
			return false;
		} else {
			return true;
		}
	} else if(x < 0 && y < 0) { //why would both of these be negative?
		if(y >= min - x)
		{
			return false;
		} else {
			return true;
		}
	} else {
		return false;
	}
}

// This function returns a vector of strings, which is the prompt split by the delim.
int split(const std::string &s, char delim, std::vector<std::string> &elems) 
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) 
    {
        elems.push_back(item);
    }
    return elems.size();
}

void buildPacket(char* packet, std::string command)
{
	packet[0] = '\0';
    //Build out nonce here
	//
	
	//Check if command overflows
	//change 1023 to variable amount based on nonce once implemented
	if(command.size() < 1023)
	{
    	strcpy(packet, (command + '\0').c_str());
    	packet[command.size()] = '\0';
	} //end if command does not overflow

}

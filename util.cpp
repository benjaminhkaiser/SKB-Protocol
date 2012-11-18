#include <util.h>
#include <string>
#include <sstream>
#include <limits>
#include <algorithm>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "includes/cryptopp/sha.h"
#include "includes/cryptopp/hex.h"

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
	} else if(x < 0 && y < 0) {
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

bool sendPacket(long int &csock, void* packet)
{
	int length = 0;

	length = strlen((char*)packet);
	printf("Send packet length: %d\n", length);
	if(sizeof(int) != send(csock, &length, sizeof(int), 0))
	{
	    printf("[error] fail to send packet length\n");
	   	return false;
	}
	if(length != send(csock, packet, length, 0))
	{
	    printf("[error] fail to send packet\n");
	    return false;
	}

	return true;
}

bool listenPacket(long int &csock, char* packet)
{
	int length;

    packet[0] = '\0';
	//read the packet from the sender
	if(sizeof(int) != recv(csock, &length, sizeof(int), 0)){
	    return false;
	}
	if(length >= 1024)
	{
	    printf("[error] packet to be sent is too long\n");
	    return false;
	}
	if(length != recv(csock, packet, length, 0))
	{
	    printf("[error] fail to read packet\n");
	    return false;
	}
	packet[length] = '\0';

	return true;
}

bool isDouble(std::string questionable_string)
{
	long double value = strtold(questionable_string.c_str(), NULL);
	if(value == 0)
	{
		return false;
	} //end if no valid conversion
	return true;
} //end isDouble function
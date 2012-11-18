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
#include <fstream>
#include "includes/cryptopp/sha.h"
#include "includes/cryptopp/hex.h"
#include "includes/cryptopp/aes.h"
#include "includes/cryptopp/ccm.h"

using CryptoPP::AES;
using CryptoPP::CCM;

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
	double max = std::numeric_limits<long double>::max();
	double min = std::numeric_limits<long double>::min();

	//x is generally the account balance
	//y is generally the amount to change by

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
	
	//Check if command overflows
	//change 1023 to variable amount based on nonce once implemented
	if(command.size() < 1023)
	{
    	strcpy(packet, (command + '\0').c_str());
    	packet[command.size()] = '\0';
	} //end if command does not overflow

}
//Takes the socket and packet and sends the packet
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

//Listens for a packet and modifies the packet variable accordingly
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

void encryptPacket(void* packet, std::string key_file)
{
	std::string key;
	std::ifstream input_key_file(key_file.c_str());

	if(input_key_file.is_open())
	{
		input_key_file >> key;
	} //end if file is open

	CryptoPP::StringSource(key, true,
					new CryptoPP::HexEncoder(
							new CryptoPP::StringSink(key))
					);	//end StringSource
	input_key_file.close();
} //end encryptPacket function

void decryptPacket(void* packet)
{
} //end decryptPacket function
void generateRandomKey(std::string name, byte* key, long unsigned int length)
{
	CryptoPP::AutoSeededRandomPool prng;

	//byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
	prng.GenerateBlock(key, length);

	std::string encoded;

	CryptoPP::StringSource(key, length, true,
		new CryptoPP::HexEncoder(
			new CryptoPP::StringSink(encoded)
		) // HexEncoder
	); // StringSource

		std::ofstream outfile;

	std::string keyFile = "keys/" + name + ".key";

	std::ofstream file_out(keyFile.c_str());
	if(file_out.is_open())
	{
		file_out << encoded;
	} //end if valid outfstream
	file_out.close();
}

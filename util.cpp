#include <util.h>
#include <string>
#include <stdio.h>
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
#include "includes/cryptopp/gcm.h"
#include "includes/cryptopp/osrng.h"

using CryptoPP::GCM;
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

bool encryptPacket(char* packet, byte* aes_key)
{
	std::string plainkey;
	CryptoPP::StringSource(aes_key, CryptoPP::AES::DEFAULT_KEYLENGTH, true,
		new CryptoPP::HexEncoder(
			new CryptoPP::StringSink(plainkey)
		) // HexEncoder
	);
	try
	{
		std::string plaintext(packet);

		//Decode the key from the file
		GCM< AES >::Encryption p;
		//iv will help us with keying out cipher
		//it is also randomly generated
		byte iv[ AES::BLOCKSIZE ];
		CryptoPP::AutoSeededRandomPool prng;
		prng.GenerateBlock( iv, sizeof(iv) );

		//Merge the iv and key
		p.SetKeyWithIV( aes_key, CryptoPP::AES::DEFAULT_KEYLENGTH, iv, sizeof(iv) );

		//Encode the IV
		std::string encoded_iv;
		CryptoPP::StringSource(iv, sizeof(iv), true,
			new CryptoPP::HexEncoder(
				new CryptoPP::StringSink(encoded_iv)
			) // HexEncoder
		);

		//Create the ciphertext from the plaintext
		std::string ciphertext;
		CryptoPP::StringSource(plaintext, true,
			new CryptoPP::AuthenticatedEncryptionFilter(p,
				new CryptoPP::StringSink(ciphertext)
			)
		);

		//Encode the cipher to be sent
		std::string encodedCipher;
		CryptoPP::StringSource(ciphertext, true,
			new CryptoPP::HexEncoder(
				new CryptoPP::StringSink(encodedCipher)
			) // HexEncoder
		);

		//replace the packet with the econded ciphertext
		strcpy(packet, (encoded_iv+encodedCipher).c_str());
		packet[(encoded_iv+encodedCipher).size()] = '\0';
	}
	catch(std::exception e)
	{
		return false;
	}
	
	return true;
} //end encryptPacket function

bool decryptPacket(char* packet, byte* aes_key)
{
	try
	{
		//Setup the iv to be retrieved
		byte iv[ AES::BLOCKSIZE];
		std::string iv_string = std::string(packet).substr(0,32);

		//Decode the iv
		CryptoPP::StringSource(iv_string, true,
			new CryptoPP::HexDecoder(
				new CryptoPP::ArraySink(iv,CryptoPP::AES::DEFAULT_KEYLENGTH)
			)
		);

		//Decode the ciphertext
		std::string ciphertext;
		CryptoPP::StringSource(std::string(packet).substr(32), true,
			new CryptoPP::HexDecoder(
				new CryptoPP::StringSink(ciphertext)
			) // HexEncoder
		);

		GCM< AES >::Decryption d;
		d.SetKeyWithIV( aes_key, CryptoPP::AES::DEFAULT_KEYLENGTH, iv, sizeof(iv));
		
		//Decrypt the ciphertext into plaintext
		std::string plaintext;
	    CryptoPP::StringSource s(ciphertext, true, 
			new CryptoPP::AuthenticatedDecryptionFilter(d,
				new CryptoPP::StringSink(plaintext)
			) // StreamTransformationFilter
		);

		//Replace the packet with the plaintext
		strcpy(packet, plaintext.c_str());
		packet[plaintext.size()] = '\0';
	}
	catch(std::exception e)
	{
		return false;
	}

	return true;
} //end decryptPacket function
void generateRandomKey(std::string name, byte* key, long unsigned int length)
{

	CryptoPP::AutoSeededRandomPool prng;

	prng.GenerateBlock(key, length);

	std::string encoded;

	//Encode the random key
	CryptoPP::StringSource(key, length, true,
		new CryptoPP::HexEncoder(
			new CryptoPP::StringSink(encoded)
		) // HexEncoder
	); // StringSource

	//Write the key file
	std::ofstream outfile;
	std::string keyFile = "keys/" + name + ".key";
	std::ofstream file_out(keyFile.c_str());
	if(file_out.is_open())
	{
		file_out << encoded;
	} //end if valid outfstream
	file_out.close();
}

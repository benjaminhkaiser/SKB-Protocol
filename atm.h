#ifndef __bank_h__
#define __bank_h__

#include <string.h>

struct AtmSession
{
	//Construct
	AtmSession() : state(0) {}

	//Functions
	bool handshake(long int &csock);
	bool sendP(long int &csock, void* packet, std::string command);
	bool listenP(long int &csock, char* packet);

	//Variables
	unsigned int state;
	std::string bankNonce;
	std::string atmNonce;
	byte* key;
};

#endif

#ifndef __bank_h__
#define __bank_h__

#include <string.h>
#include <vector>
#include "account.h"


class Bank
{
public:
	void addAccount(Account* account);
	Account* getAccountByName(const std::string& username);
	Account* connectToAccount();
	Account* tryLoginHash(const std::string& hash);
	std::string appSalt;
	~Bank();
	std::vector<byte*> keys;
	std::vector<bool> keysInUse;
private:
	std::vector<Account*> accounts;
};

struct BankSocketThread
{
	Bank* bank;
	int *csock;
};

struct BankSession
{
	//Construct
	BankSession() : state(0),error(false),account(0) {}

	//Functions
	bool sendP(long int &csock, void* packet, std::string command);
	bool validateNonce(std::string packet);
	void endSession();

	//Variables
	Account* account;
	unsigned int state;
	Bank* bank;
	bool error;
	std::string bankNonce;
	std::string atmNonce;
	byte* key;

};

#endif
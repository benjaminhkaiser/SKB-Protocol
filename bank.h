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
	Account* account;
	unsigned int state;
	bool error;
	BankSession() : state(0),error(false),account(0) {}
};

#endif
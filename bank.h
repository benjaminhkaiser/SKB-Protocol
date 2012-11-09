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

#endif
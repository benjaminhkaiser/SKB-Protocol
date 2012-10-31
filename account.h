#ifndef __account_h__
#define __account_h__
#include <string>
#include "includes/cryptopp/sha.h"

class Account
{
public:
	//Construct
	Account();

	//Transfer functions
	bool tryTransfer(double funds, const Account* toAccount) const;
	double Transfer(double funds, Account* toAccount);

	//Withdraw functions
	bool tryWithdraw(double funds) const;
	double Withdraw(double funds);

	//Deposit functions
	bool tryDeposit(double funds) const;
	double Deposit(double funds);

	//Other functions
	bool setPIN(const std::string& pin, const std::string& appSalt);
	bool createAccount(const std::string& accountHolder, const int& accountNum, const std::string& pin, const std::string& appSalt);
	bool tryLogin(const std::string& pin, const std::string& appSalt);
private:
	std::string hash;
	std::string accountHolder;
	std::string salt;
	std::string card;
	int accountNum;
	double balance;
	double withdrawLimitRemaining;
	int transferAttemptsRemaining;
	int failsRemaining;
	bool locked;

	void registerFail();
};
#endif
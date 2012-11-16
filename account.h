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
	bool tryTransfer(long double funds, const Account* toAccount) const;
	long double Transfer(long double funds, Account* toAccount);

	//Withdraw functions
	bool tryWithdraw(long double funds) const;
	long double Withdraw(long double funds);

	//Deposit functions
	bool tryDeposit(long double funds) const;
	long double Deposit(long double funds);

	//Other functions
	const long double getBalance(){ return this->balance; }
	const std::string getAccountHolder(){ return this->accountHolder; }
	bool setPIN(const std::string& pin, const std::string& appSalt);
	bool createAccount(const std::string& accountHolder, const int& accountNum, const std::string& pin, const std::string& appSalt);
	bool tryLogin(const std::string& pin, const std::string& appSalt);
	bool tryHash(const std::string& attemptedHash);
private:
	std::string hash;
	std::string accountHolder;
	std::string salt;
	std::string card;
	int accountNum;
	long double balance;
	long double withdrawLimitRemaining;
	int transferAttemptsRemaining;
	int failsRemaining;
	bool locked;

	void registerFail();
};
#endif

#include "account.h"
#include <string.h>
#include <fstream>
#include <util.h>
#include <iostream>

/*
 * Constuct
 */

Account::Account()
{
	locked = true;
	failsRemaining = 3;
	transferAttemptsRemaining = 3;
	withdrawLimitRemaining = 1000;
	balance = 0;
	accountNum = 0;
	hash = "";
}

/*
 * Transfer
 */
bool Account::tryTransfer(long double funds, const Account* toAccount) const
{
	if(!tryWithdraw(funds))
	{
		return false;
	}


	if(!toAccount->tryDeposit(funds))
	{
		return false;
	}

	return true;
}
long double Account::Transfer(long double funds, Account* toAccount)
{
	if(funds == 0)
	{
		return this->balance;
	}

	if(tryTransfer(funds, toAccount))
	{
		Withdraw(funds);
		toAccount->Deposit(funds);
	}

	return this->balance;
}

/*
 * Withdraw
 */

bool Account::tryWithdraw(long long double funds) const
{
	if(funds < 0)
	{
		return false;
	}

	//You cannot exceed your withdrawal limit
	if(funds > this->withdrawLimitRemaining)
	{
		return false;
	}

	//Let's not overflow now!
	if(doubleOverflow(funds*-1,this->balance))
	{
		return false;
	}

	//We don't allow overdraft
	if(this->balance - funds < 0)
	{
		return false;
	}

	return true;
}

long double Account::Withdraw(long double funds)
{
	if(tryWithdraw(funds))
	{
		this->balance -= funds;
	}

	return this->balance;
}

/*
 * Deposit
 */

bool Account::tryDeposit(long double funds) const
{
	if(funds < 0)
	{
		return false;
	}

	if(!doubleOverflow(funds,this->balance))
	{
		return true;
	} else {
		return false;
	}
}
long double Account::Deposit(long double funds)
{
	if(tryDeposit(funds))
	{
		this->balance += funds;

	}
	return this->balance;
}


/*
 * Other functions
 */

bool Account::createAccount(const std::string& accountHolder, const int& accountNum, const std::string& pin, const std::string& appSalt)
{
	if(accountHolder == "")
	{
		return false;
	}
	this->accountHolder = accountHolder;
	if(accountNum <= 0)
	{
		return false;
	}
	this->accountNum = accountNum;

	this->salt = makeHash(randomString(128));

	std::string card = makeHash(to_string(this->accountNum) + salt);

	this->card = card;

	std::ofstream outfile;

	std::string cardFile = "cards/" + this->accountHolder + ".card";

	std::ofstream file_out(cardFile.c_str());
	if(file_out.is_open())
	{
		file_out << card;
	} //end if valid outfstream
	file_out.close();
	//outfile.open (cardFile.c_str(), std::ios_base::out|std::ios_base::trunc);
	//if(outfile.is_open())
	//{
	//	outfile << card;
	//}
	//outfile.close();

	//If you successfully set the pin then the account can be unlocked for use.
	if(setPIN(pin, appSalt))
	{
		locked = false;
	} else {
		locked = true;
	}

	return true;
}

bool Account::setPIN(const std::string& pin, const std::string& appSalt)
{
	//require pin length of at least 6 but no more than 32
	if(pin.length() < 6 || pin.length() > 32)
	{
		return false;
	}
	std::string hash = makeHash(this->card + pin + appSalt);

	//verify that hash was actually created
	if(hash.length() > 0)
	{
		this->hash = hash;
		return true;
	} else {
		return false;
	}
}

bool Account::tryLogin(const std::string& pin, const std::string& appSalt)
{
	if(this->locked)
	{
		return false;
	}

	std::string attemptedHash = makeHash(this->card + pin + appSalt);

	if(this->hash == attemptedHash)
	{
		return true;
	} else {
		registerFail();
		return false;
	}
}

bool Account::tryHash(const std::string& attemptedHash)
{
	if(this->locked)
	{
		return false;
	}
	if(this->hash == attemptedHash)
	{
		return true;
	} else {
		registerFail();
		return false;
	}	
}

void Account::registerFail()
{
	if(failsRemaining > 1)
	{
		this->failsRemaining -= 1;
	} else {
		this->locked = true;
	}
}

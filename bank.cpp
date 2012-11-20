/**
    @file bank.cpp
    @brief Top level bank implementation file
 */
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include "account.h"
#include "bank.h"
#include "util.h"

void* client_thread(void* arg);
void* console_thread(void* arg);

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        printf("Usage: bank listen-port\n");
        return -1;
    }
    
    unsigned short ourport = atoi(argv[1]);
    
    //socket setup
    int lsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(!lsock)
    {
        printf("fail to create socket\n");
        return -1;
    }
    
    //listening address
    sockaddr_in addr_l;
    memset(&addr_l,0,sizeof(addr_l));
    addr_l.sin_family = AF_INET;
    addr_l.sin_port = htons(ourport);
    unsigned char* ipaddr = reinterpret_cast<unsigned char*>(&addr_l.sin_addr);
    ipaddr[0] = 127;
    ipaddr[1] = 0;
    ipaddr[2] = 0;
    ipaddr[3] = 1;
    if(0 != bind(lsock, reinterpret_cast<sockaddr*>(&addr_l), sizeof(addr_l)))
    {
        printf("failed to bind socket\n");
        return -1;
    }
    if(0 != listen(lsock, SOMAXCONN))
    {
        printf("failed to listen on socket\n");
        return -1;
    }

    //Create the bank
    Bank* bank = new Bank();

    //Create the structs to help move data around
    BankSocketThread* bankSocketThread = new BankSocketThread();
    bankSocketThread->bank = bank;
    bank->appSalt = "THISISASUPERSECUREAPPWIDESALT";
    
    pthread_t cthread;
    pthread_create(&cthread, NULL, console_thread, (void*)bankSocketThread);

    //loop forever accepting new connections
    while(1)
    {
        sockaddr_in unused;
        socklen_t size = sizeof(unused);
        int csock = accept(lsock, reinterpret_cast<sockaddr*>(&unused), &size);
        if(csock < 0)   //bad client, skip it
            continue;
        bankSocketThread->csock = &csock;
        pthread_t thread;
        pthread_create(&thread, NULL, client_thread, (void*)bankSocketThread);
    }
    delete bank;
    delete bankSocketThread;
}

void* client_thread(void* arg)
{
    BankSocketThread* bankSocketThread = (BankSocketThread*) arg;
    Bank* bank = bankSocketThread->bank;
    BankSession* bankSession = new BankSession();
    bankSession->state = 0;
    bankSession->bank = bank;
    bankSession->key = 0;

    long int csock = (long int)*(bankSocketThread->csock);
    
    printf("[bank] client ID #%ld connected\n", csock);
    
    //input loop
    int length;
    char packet[1024];
    bool fatalError = false;
    std::vector<std::string> tokens;
    while(1)
    {
        fatalError = false;
        tokens.clear();
        
        if(!listenPacket(csock, packet))
        {
            printf("[bank] fail to read packet\n");
            break;
        }

        if(!bankSession->key)
        {
            if(bankSession->state != 0)
            {
                printf("[error] Unexpectd state\n");
                break;
            }
            for(unsigned int i = 0; i < bank->keys.size(); ++i)
            {
                if(bank->keysInUse[i])
                {
                    continue;
                }
                if(decryptPacket((char*)packet,bank->keys[i])
                    && std::string(packet).substr(0,9) == "handshake")
                {
                    bankSession->key = bank->keys[i];
                    bank->keysInUse[i] = true;
                    break;
                }
            }
            if(!bankSession->key)
            {
                printf("[error] Key not found.\n");
                break;
            }
        } else {
            if(!decryptPacket((char*)packet, bankSession->key))
            {
                printf("[error] Invalid key\n");
                break;
            }
        }    

        //Parse the packet
        //std::string strPacket = packet;
        split(std::string(packet),',', tokens);

        //We should get something, if not ignore this packet
        if(tokens.size() < 1)
        {
            continue;
        }

        if(tokens[0] == "logout")
        {
            bankSession->endSession();
            break;
        }

        //Now we're compare what we go to what state we expect to be in
        switch(bankSession->state)
        {
            case 0:
            case 1:
                if(tokens.size() == 2 && tokens[0] == "handshake" && tokens[1].size() == 128)
                {
                    bankSession->atmNonce = tokens[1];
                    bankSession->bankNonce = makeHash(randomString(128));
                    if(bankSession->bankNonce.size() == 0)
                    {
                        printf("Unexpected error\n");
                        fatalError = true;
                        break;
                    }
                    buildPacket(packet, "handshakeResponse," + bankSession->atmNonce + "," + bankSession->bankNonce);
                    if(!encryptPacket((char*)packet,bankSession->key))
                    {
                        printf("Unexpected error\n");
                        fatalError = true;
                        break;
                    }
                    if(!sendPacket(csock, packet))
                    {
                        printf("Unexpected error\n");
                        fatalError = true;
                        break;
                    }
                    bankSession->state = 2;
                }
                break;
            //Expecting a login
            case 2:
                if(!bankSession->validateNonce(std::string(packet)))
                {
                    printf("Unexpected error\n");
                    fatalError = true;
                    break;
                }
                if(tokens.size() == 5 && tokens[0] == "login" && tokens[1].size() == 128)
                {
                    //Now we'll try to find the account
                    //bankSession->account = bank->tryLoginHash(tokens[1]);
                    bankSession->account = bank->getAccountByName(tokens[2]);
                    if(!bankSession->account || !bankSession->account->tryHash(tokens[1]))
                    {
                        //Failed login
                        //TODO Blacklist hash
                        bankSession->error = true;
                        printf("[notice] Failed login!\n");
                    }
                    bankSession->account->inUse = true;
                    bankSession->state = 5;
                    if(!bankSession->sendP(csock,packet, "ack"))
                    {
                        printf("Unexpected error!\n");
                        fatalError = true;
                        break;
                    }
                }
                break;
            case 5:
                bool returnBalance = false;
                bankSession->state = 4;
                if(bankSession->error)
                {
                    returnBalance = false;
                } 
                else if(tokens.size() == 3 && tokens[0] == "balance")
                {
                    returnBalance = true;
                }
                else if(tokens.size() == 4 && tokens[0] == "withdraw" && isDouble(tokens[1]))
                {
                    double amount = atof(tokens[1].c_str());
                    if(!bankSession->account->Withdraw(amount))
                    {
                        printf("[error] Failed withdraw\n");
                        returnBalance = false;
                        bankSession->error = true;
                    }
                    returnBalance = true;
                }
                else if(tokens.size() == 5 && tokens[0] == "transfer" && !isDouble(tokens[1])
                    && isDouble(tokens[2]))
                {
                    Account* accountTo = bank->getAccountByName(tokens[1]);
                    double amount = atof(tokens[2].c_str());
                    if(!bankSession->account->Transfer(amount, accountTo))
                    {
                        printf("[error] Failed transfer\n");
                        returnBalance = false;
                        bankSession->error = true;
                    }
                    returnBalance = true;
                }

                if(bankSession->error)
                {
                    bankSession->sendP(csock, packet, "denied");
                }
                else if(returnBalance)
                {
                    char moneyStr[256];
                    sprintf(moneyStr,"%.2Lf",bankSession->account->getBalance());
                    bankSession->sendP(csock, packet, std::string(moneyStr));
                }

                //Reset back to initial state
                bankSession->endSession();
                break;
        }
        
        if(fatalError)
        {
            bankSession->endSession();
            break;
        }
    }

    bankSession->endSession();

    printf("[bank] client ID #%ld disconnected\n", csock);

    close(csock);
    delete bankSession;
    return NULL;
}

void* console_thread(void* arg)
{
    BankSocketThread* bankSocketThread = (BankSocketThread*) arg;
    Bank* bank = bankSocketThread->bank;

    //Let's generate our keys
    for(unsigned int i = i; i < 50; ++i)
    {
        byte* key = new byte[CryptoPP::AES::DEFAULT_KEYLENGTH];
        generateRandomKey(to_string((int)i),key, CryptoPP::AES::DEFAULT_KEYLENGTH);
        bank->keys.push_back(key);
        bank->keysInUse.push_back(false);
    }

    //Create Accounts
    Account* new_account = new Account();

    //Alice
    new_account->createAccount(std::string("alice"), 1, std::string("123456"), bank->appSalt);
    new_account->Deposit(100);
    bank->addAccount(new_account);

    //Bob
    new_account = new Account();
    new_account->createAccount(std::string("bob"), 2, std::string("234567"), bank->appSalt);
    new_account->Deposit(50);
    bank->addAccount(new_account);

    //Eve
    new_account = new Account();
    new_account->createAccount(std::string("eve"), 3, std::string("345678"), bank->appSalt);
    new_account->Deposit(0);
    bank->addAccount(new_account);

    char buf[80];
    while(1)
    {
        printf("bank> ");
        fgets(buf, 79, stdin);
        buf[strlen(buf)-1] = '\0';  //trim off trailing newline

        std::vector<std::string> tokens;
        split(buf,' ',tokens);

        if(tokens.size() <= 0)
        {
            printf("Invalid input\n");
            continue;
        }
        if(tokens[0] == "balance")
        {
            if(tokens.size() != 2)
            {
                printf("Invalid input\n");
                continue;
            }

            Account* current_account = bank->getAccountByName(tokens[1]);
            if(!current_account)
            {
                printf("Invalid account\n");
                continue;
            }
            printf("Balance: %.2Lf\n", current_account->getBalance());
            continue;
        }

        if(tokens[0] == "deposit")
        {
            if(tokens.size() != 3)
            {
                printf("Invalid input\n");
                continue;
            }

            long double amount = atof(tokens[2].c_str());

            if(amount <= 0)
            {
                printf("Invalid amount\n");
                continue;
            }

            Account* current_account = bank->getAccountByName(tokens[1]);
            if(!current_account)
            {
                printf("Invalid account\n");
                continue;
            }

            if(current_account->Deposit(amount))
            {
                long double cur_balance = current_account->getBalance();
                printf("Money deposited!\nNew balance: %.2Lf\n", cur_balance);
            } else {
                printf("Error depositing money!\n");
            }
            continue;
        }
        
    }
}

void Bank::addAccount(Account* account)
{
    this->accounts.push_back(account);
}

Account* Bank::getAccountByName(const std::string& username)
{
    for(unsigned int i = 0; i < this->accounts.size(); ++i)
    {
        if(this->accounts[i]->getAccountHolder() == username)
        {
            return this->accounts[i];
        }
    }
    return 0;
}

Account* Bank::tryLoginHash(const std::string& hash)
{
    for(unsigned int i = 0; i < this->accounts.size(); ++i)
    {
        if(this->accounts[i]->tryHash(hash))
        {
            return this->accounts[i];
        }
    }
    return 0;
}

Bank::~Bank()
{
    for(unsigned int i = 0; i < this->accounts.size(); ++i)
    {
        delete this->accounts[i];
    }

    for(unsigned int i = 0; i < this->keys.size(); ++i)
    {
        delete this->keys[i];
    }
}


bool BankSession::sendP(long int &csock, void* packet, std::string command)
{
    if(!this->key)
    {
        return false;
    }

    bankNonce = makeHash(randomString(128));
    command = command + "," + atmNonce + "," + bankNonce;
    buildPacket((char*)packet, command);
    if(!encryptPacket((char*)packet,this->key))
    {
        return false;
    }

    return sendPacket(csock, packet);
}

bool BankSession::validateNonce(std::string packet)
{
    try
    {
        if(packet.substr(packet.size()-128, 128) != bankNonce)
        {
            return false;
        }
        atmNonce = packet.substr(packet.size()-257, 128);

        return true;
    }
    catch(std::exception e)
    {
        return false;
    }
}

void BankSession::endSession()
{
    if(this->account)
    {
        this->account->inUse = false;
    }
    this->account = 0;
    if(this->key)
    {
        for(unsigned int i = 0; i < this->bank->keys.size(); ++i)
        {
            if(this->key == this->bank->keys[i])
            {
                this->bank->keysInUse[i] = false;
            }
        }
    }
    this->key = 0;
    this->state = 0;
}

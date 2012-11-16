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
    bankSession->state = 2;

    long int csock = (long int)*(bankSocketThread->csock);
    
    printf("[bank] client ID #%ld connected\n", csock);
    
    //input loop
    int length;
    char packet[1024];
    bool sendPacket = false;
    std::vector<std::string> tokens;
    while(1)
    {
        sendPacket = false;
        tokens.clear();
        packet[0] = '\0';
        //read the packet from the ATM
        if(sizeof(int) != recv(csock, &length, sizeof(int), 0)){
            break;
        }
        if(length >= 1024)
        {
            printf("packet too long\n");
            break;
        }
        if(length != recv(csock, packet, length, 0))
        {
            printf("[bank] fail to read packet\n");
            break;
        }
        packet[length] = '\0';
        
        //Temporary code for debugging: print received packet
        printf("%s", packet);
        printf("\n");

        //Parse the packet
        //std::string strPacket = packet;
        split(std::string(packet),',', tokens);

        //We should get something, if not ignore this packet
        if(tokens.size() < 1)
        {
            continue;
        }
        //Now we're compare what we go to what state we expect to be in
        switch(bankSession->state)
        {
            //We're not doing nonces yet so we can skip first two states
            case 0:
            case 1:
            //Expecting a login
            case 2:
                if(tokens.size() == 2 && tokens[0] == "login" && tokens[1].size() == 128)
                {
                    //Now we'll try to find the account
                    bankSession->account = bank->tryLoginHash(tokens[1]);
                    if(!bankSession->account)
                    {
                        //Failed login
                        //TODO Blacklist hash
                        bankSession->error = true;
                        printf("Failed login!\n");
                    }
                    bankSession->state = 5;
                    buildPacket(packet, "ack");
                    sendPacket = true;
                }
                break;
            case 5:
                if(bankSession->error)
                {
                    buildPacket(packet, "Transaction denied.");
                    sendPacket = true;
                } 
                else if(tokens.size() == 1 && tokens[0] == "balance")
                {
                    bankSession->state = 4;
                    char moneyStr[256];
                    sprintf(moneyStr,"%.2f",bankSession->account->getBalance());
                    buildPacket(packet, std::string(moneyStr));
                    sendPacket = true;
                }
                break;
        }
        
        //TODO: process packet data
        
        //TODO: put new data in packet
        
        //send the new packet back to the client
        if(sendPacket)
        {
            length = strlen(packet);
            printf("Send packet length: %d\n", length);
            if(sizeof(int) != send(csock, &length, sizeof(int), 0))
            {
                printf("[bank] fail to send packet length\n");
                break;
            }
            if(length != send(csock, (void*)packet, length, 0))
            {
                printf("[bank] fail to send packet\n");
                break;
            }
        }
    }

    printf("[bank] client ID #%ld disconnected\n", csock);

    close(csock);
    delete bankSession;
    return NULL;
}

void* console_thread(void* arg)
{
    BankSocketThread* bankSocketThread = (BankSocketThread*) arg;
    Bank* bank = bankSocketThread->bank;

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
        printf("%s of size: %d\n", tokens[1].c_str(), (int)tokens[1].size());
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
            printf("Balance: %f\n", current_account->getBalance());
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

            if(!current_account->tryDeposit(amount))
            {
                printf("Invalid amount\n");
                continue;
            }

            long double cur_balance = current_account->Deposit(amount);

            printf("Money deposited!\nNew balance: %f\n", cur_balance);
            continue;
        }

        //printf("%s", buf);
        //printf("\n");
        
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
}

/**
    @file atm.cpp
    @brief Top level ATM implementation file
 */
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include <termios.h>
#include "util.h"

using std::cout;
using std::cin;
using std::endl;

void buildPacket(char* packet, std::string command);

//Helper function for getpass() It reads in each character to be masked.
int getch() {
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}

// This function returns a vector of strings, which is the prompt split by the delim.
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) 
{
    std::stringstream ss(s+' ');
    std::string item;
    while(std::getline(ss, item, delim)) 
    {
        elems.push_back(item);
    }
    return elems;
}

//This function prompts for and receives the user-entered PIN (masked with *'s)
std::string getpass(const char *prompt, bool show_asterisk=true){
    const char BACKSPACE=127;
    const char RETURN=10;

    std::string password;
    unsigned char ch=0;

    cout << prompt;
    while((ch=getch())!=RETURN){
        if(ch==BACKSPACE){
            if(password.length()!=0){
                if(show_asterisk)
                    cout <<"\b \b";
                password.resize(password.length()-1);
            }
        } //end if BACKSPACE
        else{
            password+=ch;
            if(show_asterisk)
                cout <<'*';
        } //end else
    } //end while

  printf("\n");
  
  return password;
}


int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        printf("Usage: atm proxy-port\n");
        return -1;
    }
 
    //Set the appSalt
    const std::string appSalt = "THISISASUPERSECUREAPPWIDESALT";

    //socket setup
    unsigned short proxport = atoi(argv[1]);
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(!sock)
    {
        printf("fail to create socket\n");
        return -1;
    }
    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(proxport);
    unsigned char* ipaddr = reinterpret_cast<unsigned char*>(&addr.sin_addr);
    ipaddr[0] = 127;
    ipaddr[1] = 0;
    ipaddr[2] = 0;
    ipaddr[3] = 1;
    if(0 != connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)))
    {
        printf("fail to connect to proxy\n");
        return -1;
    }
    
    //input loop   
    while(1)
    {
        char buf[80];
        char packet[1024];
        int length;
        int sendPacket = 0;
        std::vector<std::string> bufArray;

        // clean up last packet and buffer
        buf[0] = '\0';
        packet[0] = '\0';

        // Print the prompt
        printf("atm> ");
        fgets(buf, 79, stdin);
        buf[strlen(buf)-1] = '\0';  //trim off trailing newline

        //TODO: We need to get a nonce from the server.. and make sure the server is who we think it is
        
        // Parse data
        bufArray = split((std::string) buf, ' ', bufArray);

        //input parsing
        if(bufArray.size() >= 1 && ((std::string) "") != bufArray[0])
        {
            std::string command = bufArray[0];
                
            // There exists a command, check the command
            if(!strcmp(buf, "logout"))
            {   
                sendPacket = 1; // Send packet because valid command
                break;
            }
            else if(((std::string) "login") == command) //if command is 'login'
            {   
                //this block prompts for 30 char username for login and puts it in the username var
                // Continue as long as there is only one argument.
                if(bufArray.size() == 2)
                {
                    //limit username to 30 chars
                    std::string username = bufArray[1].substr(0,30);
                    std::ifstream cardFile(("cards/" + username + ".card").c_str());
                    if(cardFile)
                    {
                        sendPacket = 1; // Send packet because valid command

                        //obtain card hash
                        std::string cardHash((std::istreambuf_iterator<char>(cardFile)),std::istreambuf_iterator<char>());
                        cardHash = cardHash.substr(0,128);
                        cout << "Card: " << cardHash << '\n';

                        //this block prompts for PIN for login and puts it in the pin var
                        std::string pin;
                        pin = getpass("PIN: ", true);
                        //Pin is limited to 6 characters
                        pin = pin.substr(0,6);

                        //Now we'll figure out the hash that we need to send
                        std::string accountHash = makeHash(cardHash + pin + appSalt);
                      
                        //This block takes the info the user input and puts it into a packet.
                        //The packet looks like: login,[username],[username.card account hash],[PIN]
                        buildPacket(packet,std::string(command + ',' + accountHash + '\0'));
                        //strcpy(packet,(command + ',' + accountHash + '\0').c_str());
                    }
                    else
                    {
                        cout << "ATM Card not found.\n";
                    }

                }
                else
                {
                    cout << "Usage: login [username]\n";
                }
            } 

            //TODO: other commands
            
            else
            {
                cout << "Command '" << command << "' not recognized.\n";
            }

            if(sendPacket)
            {
                //This block sends the message through the proxy to the bank. 
                //There are two send messages - 1) packet length and 2) actual packet
                length = strlen(packet);
                cout << "Plen: " << length << endl;
                if(sizeof(int) != send(sock, &length, sizeof(int), 0))
                {
                    printf("fail to send packet length\n");
                    break;
                }
                if(length != send(sock, (void*)packet, length, 0))
                {
                    printf("fail to send packet\n");
                    break;
                }
                
                //TODO: do something with response packet
                if(sizeof(int) != recv(sock, &length, sizeof(int), 0))
                {
                    printf("fail to read packet length\n");
                    break;
                }
                if(length >= 1024)
                {
                    printf("packet too long\n");
                    break;
                }
                if(length != recv(sock, packet, length, 0))
                {
                    printf("fail to read packet\n");
                    break;
                }
            }
        }
        else
        {
            cout << "Usage: [command] [+argument]\n";
        } 
    }
    
    //cleanup
    close(sock);
    return 0;
}

void buildPacket(char* packet, std::string command)
{
    //Build out nonce here
    strcpy(packet, command.c_str());
}
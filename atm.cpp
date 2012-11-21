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
#include "atm.h"
#include <exception>

using std::cout;
using std::cin;
using std::endl;

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
    if(argc != 3)
    {
        printf("Usage: atm proxy-port atm-number(1-50)\n");
        return -1;
    }
 
    //Set the appSalt
    const std::string appSalt = "THISISASUPERSECUREAPPWIDESALT";

    //socket setup
    unsigned short proxport = atoi(argv[1]);
    long int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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

    AtmSession atmSession = AtmSession();
    atmSession.key = 0;
    
	//construct filename from command-line argument
	std::string filename = "keys/" + std::string(argv[2]) + ".key";	
	
	//read in key from file
	std::string key;
	std::ifstream input_file(filename.c_str());
	if(input_file.is_open()){
		input_file >> key;
	}
	input_file.close();

	byte atm_key[CryptoPP::AES::DEFAULT_KEYLENGTH];

	//assign to atmSessions    	
	CryptoPP::StringSource(key, true,
		new CryptoPP::HexDecoder(
			new CryptoPP::ArraySink(atm_key,CryptoPP::AES::DEFAULT_KEYLENGTH)
		)
	);
    atmSession.key = atm_key;	

	//input loop   
    while(1)
    {
        char buf[80];
        char packet[1024];
        int length;
        int sendPacket = 0;
        std::vector<std::string> bufArray;
        std::vector<std::string> tokens;

        // clean up last packet and buffer
        buf[0] = '\0';
        packet[0] = '\0';

        // Print the prompt
        printf("atm> ");
        fgets(buf, 79, stdin);
        buf[strlen(buf)-1] = '\0';  //trim off trailing newline

        // Parse data
        split((std::string) buf, ' ', bufArray);

        if(std::string(buf).size() >= 204)
        {
            printf("Invalid input. Try again.\n");
            continue;
        }

        //input parsing
        if(bufArray.size() >= 1 && ((std::string) "") != bufArray[0])
        {
            std::string command = bufArray[0];

                
            // There exists a command, check the command
            if(((std::string) "logout") == command|| ((std::string) "exit") == command)
            {   
                if(atmSession.state > 0)
                {
                    atmSession.sendP(sock,packet,"logout");
                }
				//TODO: Send logout message so that someone could log in at a different atm
                //sendPacket = 1; // Send packet because valid command
                break;
            }
            else if(((std::string) "login") == command && atmSession.state == 0) //if command is 'login'
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
                        atmSession.handshake(sock);
                        if(atmSession.state != 2)
                        {
                            cout << "Unexpected error.\n";
                            break;
                        }
                        sendPacket = 1; // Send packet because valid command

                        //obtain card hash
                        std::string cardHash((std::istreambuf_iterator<char>(cardFile)),std::istreambuf_iterator<char>());
                        cardHash = cardHash.substr(0,128);
                        cout << "Card: " << bufArray[1] << '\n';

                        //this block prompts for PIN for login and puts it in the pin var
                        std::string pin;
                        pin = getpass("PIN: ", true);
                        //Pin is limited to 6 characters
                        //pin = pin.substr(0,6);

                        //Now we'll figure out the hash that we need to send
                        std::string accountHash = makeHash(cardHash + pin + appSalt);
                      
                        //This block takes the info the user input and puts it into a packet.
                        //The packet looks like: login,[username],[username.card account hash],[PIN]
                        //buildPacket(packet,std::string(command + ',' + accountHash));
                        if(!atmSession.sendP(sock, packet,std::string("login," + accountHash + "," + username)))
                        {
                            cout << "Unexpected error.\n";
                            break;
                        }
                        atmSession.state = 3;

                        if(!atmSession.listenP(sock, packet) || std::string(packet).substr(0,3) != "ack")
                        {
                            cout << "Unexpected error.\n";
                            cout << "Expected ack but got " << std::string(packet).substr(0,3) << "\n";
                            break;
                        }
                        atmSession.state = 4;
                        //cout << "All logged in!\n";
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
            else if(((std::string) "balance") == command && atmSession.state == 4)
            {
                atmSession.sendP(sock,packet,"balance");
                atmSession.listenP(sock,packet);
                split(std::string(packet), ',',tokens);
                if(tokens[0] == "denied")
                {
                    cout << "Transaction denied.\n";
                } else {
                    printf("Transaction complete!\nCurrent balance: %s\n", tokens[0].c_str());
                }
                atmSession.state = 5;
            } //end if command is balance
            else if(((std::string) "withdraw") == command && atmSession.state == 4)
            {
                if(bufArray.size() == 2 && isDouble(bufArray[1]))
                {
                    atmSession.sendP(sock,packet,"withdraw," + bufArray[1]);
                    atmSession.listenP(sock,packet);
                    split(std::string(packet), ',',tokens);
                    if(tokens[0] == "denied")
                    {
                        cout << "Transaction denied.\n";
                    } else {
                        printf("Transaction complete!\nCurrent balance: %s\n", tokens[0].c_str());
                    }
                    atmSession.state = 5;
                } //end if correct number of args, last arg is a double
                else
                {
                    cout << "Usage: withdraw [amount]\n";
                } //end else in correct format

            } //end if command is withdraw
			else if(((std::string) "transfer") == command && atmSession.state == 4)
			{
				if(bufArray.size() == 3)
				{
					//If this command is entered correctly, then is_double
					//should be 0 and transfer_value should be both positive and
					//not 0
					long double is_double = string_to_Double(bufArray[1]);
					long double transfer_value = string_to_Double(bufArray[2]);
					if(is_double == 0 && transfer_value > 0)
					{
						atmSession.sendP(sock,packet,"transfer," + bufArray[1] + "," + bufArray[2]);
						atmSession.listenP(sock,packet);
						split(std::string(packet), ',',tokens);
						if(tokens[0] == "denied")
						{
							cout << "Transaction denied.\n";
						} else {
							printf("Transaction complete!\nCurrent balance: %s\n", tokens[0].c_str());
						}
						atmSession.state = 5;
					} //end if correct number of args, last arg is a double, second arg is not a double
				} //end if correct number of arguments
				else
				{
					cout << "Usage: transfer [target_account] [amount]\n";
				} //end else in correct format
			} //end else if command is transfer

            //TODO: other commands
            
            else
            {
                cout << "Command '" << command << "' not recognized.\n";
            }

            if(atmSession.state == 5)
            {
                break;
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

bool AtmSession::handshake(long int &csock)
{
    state = 0;

    char packet[1024];
    atmNonce = makeHash(randomString(128));

    if(atmNonce == "")
    {
        atmNonce = "";
        return false;
    }
    buildPacket(packet,"handshake," + atmNonce);
    if(!this->key)
    {
        return false;
    }
    if(!encryptPacket(packet,this->key))
    {
        atmNonce = "";
        return false;
    }
    if(!sendPacket(csock, packet))
    {
        atmNonce = "";
        return false;
    }
    state = 1;

    if(!listenPacket(csock, packet))
    {
        atmNonce = "";
        return false;
    }
    decryptPacket((char*)packet, this->key);

    std::vector<std::string> tokens;

    split(std::string(packet),',', tokens);

    if(tokens.size() < 3 || tokens[0] != "handshakeResponse" || tokens[1].size() != 128 
        || tokens[2].size() != 128 || tokens[1] != atmNonce)
    {
        atmNonce = "";
        return false;
    }

    bankNonce = tokens[2];
    state = 2;

    return true;
}
//Takes a socket, a packet and a command and generates a nonce, then it
//sends the packet with the nonce
bool AtmSession::sendP(long int &csock, void* packet, std::string command)
{
    atmNonce = makeHash(randomString(128));
    command = command + "," + atmNonce + "," + bankNonce;
    if(command.size() >= 460)
    {
        return false;
    }
    buildPacket((char*)packet, command);
	if(!encryptPacket((char*)packet, this->key))
    {
        return false;
    }
    return sendPacket(csock, packet);
}
bool AtmSession::listenP(long int &csock, char* packet)
{
    if(!listenPacket(csock,packet) || !decryptPacket((char*)packet,this->key))
    {
        return false;
    }
	try
    {
		std::string response(packet);

		if(response.substr(0, 4) == "kill")
		{
			return false;
		}

		if(response.substr(response.size()-257, 128) != atmNonce)
		{
			return false;
		}

		bankNonce = response.substr(response.size()-128, 128);

		return true;
	} //end try
	catch (std::exception e)
	{
			return false;
	} //end catch
}

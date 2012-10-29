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
#include <iostream>
#include <string>
#include <termios.h>

using std::cout;
using std::cin;
using std::endl;

//This reads in each character to be masked
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

/*
    This function prints the string "PIN: " and gets the user entered
    pin while replacing each character with '*'
*/
std::string getpass(const char *prompt, bool show_asterisk=true)
{
  const char BACKSPACE=127;
  const char RETURN=10;

  std::string password;
  unsigned char ch=0;

  cout << prompt;
  //Consume previous newline
  ch = getch();
  while((ch=getch())!=RETURN)
    {
       if(ch==BACKSPACE)
         {
            if(password.length()!=0)
              {
                 if(show_asterisk)
                 cout <<"\b \b";
                 password.resize(password.length()-1);
              }
         }
       else
         {
             password+=ch;
             if(show_asterisk)
                 cout <<'*';
         }
    }
    
  return password;
}


int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        printf("Usage: atm proxy-port\n");
        return -1;
    }
    
    //socket setup
    unsigned short proxport = atoi(argv[1]);
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(!sock)
    {
        printf("fail to create socket\n");
        return -1;
    }
    sockaddr_in addr;
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
    //****************Should have boolean for successful login********************!
    char buf[80];
    while(1)
    {
        printf("atm> ");
        fgets(buf, 79, stdin);
        buf[strlen(buf)-1] = '\0';  //trim off trailing newline
        
        //TODO: your input parsing code has to put data here
        //Make sure to check buffer overflow
        
        //login [username]
        
        char packet[1024];
        int length = 1;
        
        //input parsing
        if(!strcmp(buf, "logout"))
            break;
        else if(!strcmp(buf, "login")){    //test login functionality
            //this block prompts for 30 char username for login and puts it in the username var
            std::string username;
            cout << "user: ";
            cin >> username;
            username = username.substr(0,30);
            
            //this block prompts for PIN for login and puts it in the pin var
            std::string pin;
            pin = getpass("PIN: ", true);
            pin = pin.substr(0,4);
          
            strcpy(packet,buf);
            for(unsigned int i = 0; i < username.length(); ++i)
            {
                packet[strlen(buf) + i] = username[i];
            } //end for appened info to packet
            for(unsigned int i = 0; i < 4; ++i)
            {
                packet[strlen(buf) + username.length() + i] = pin[i];
            } //end for append pin to packet
            //Add the terminating newline
            packet[strlen(buf) + username.length() + 4] = '\0';
            //printf(packet);
           
        } //end else if login   
        
        //TODO: other commands
        
        //send the packet through the proxy to the bank
        //There are two send messages - 1) packet length and 2) actual packet
        
        length = strlen(packet);
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
    
    //cleanup
    close(sock);
    return 0;
}
/*
 * clientTCP.cpp
 *
 *  Created on: Oct 10, 2013
 *      Author: Peter
 */

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

// Socket code and error messaging based on example found at:
//      http://www.linuxhowtos.org/C_C++/socket.htm

static int PACKSIZE = 128;

void error(const char *msg)
{
    fprintf(stderr, msg);
    exit(0);
}

/**
 * sends a command through the socket, with a specified type and content
 */
void writeSocket(int sock, char type, string content)
{
    int n;
    string s;

    // put command bit in front of content and an end-of-data flag in back
    content = type + content + "!ENDDATA!";

    // write chunks of the appropriate size to the TCP stream
    do
    {
        // write PACKSIZE bytes
        s = content.substr(0, PACKSIZE-1);
        //cout<<"Writing content: "<<s<<endl;
        n = write(sock, content.c_str(), content.size());
        if (n < 0) error("ERROR writing to socket");
        // remove written part from content
        if(content.size()>PACKSIZE)
            content = content.substr(PACKSIZE, string::npos);
        else content = "";
    } while(content.size()>0);
}

/**
 * recursive helper function for fetching data from socket
 * will wait for data if the buffer is empty
 */
string readSocketHelper(int sock, float t)
{
    int n;
    char buffer[PACKSIZE];

    // grab data in socket buffer
    bzero(buffer,PACKSIZE);
    n = read(sock,buffer,PACKSIZE-1);
    if (n < 0) error("ERROR reading from socket");
    // sleep and recur if no data in socket buffer
    else if(n==0) {
        t+=0.5f;
        sleep(0.5f);
        return readSocketHelper(sock, t);
    }
    return buffer;
}

/**
 * recieves and unpackages data from the socket
 * waits for data to arrive if the socket buffer is empty
 * leftover data after termination flag is saved for next call
 */
void readSocket(int sock, char& type, string& content, string& leftovers)
{
    int i;
    string s;

    s = leftovers;
    // if there is no data in leftovers, get first part of data from socket
    if(leftovers.size()==0) s = s + readSocketHelper(sock, 0.0f);
    leftovers = ""; // clear old leftovers

    // remove command byte
    type = s[0];
    s = s.substr(1, string::npos);

    // clear data until a valid command byte is reached
    if(type != 0x01 && type != 0x02 && type != 0x03 && type != 0x04 && type != 0x05 && type != 0x06)
    {
        leftovers = s;
        readSocket(sock, type, content, leftovers);
        return;
    }

    //cout<<"Cmd: "<<type<<endl;


    // if the end-of-data flag isn't present, keep fetching more data
    i = s.find("!ENDDATA!");
    while(i == string::npos)
    {
        s = s + readSocketHelper(sock, 0.0f);
        i = s.find("!ENDDATA!");
    }

    // edit out flag, assign content data and leftover data
    leftovers = s.substr(i+9, string::npos);
    content =  s.substr(0, i);

    //cout<<"Recieving content: "<<content<<endl<<"Leftovers: "<<leftovers<<endl;
}

int main(int argc, char *argv[])
{
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    string username;

    // grab port, username from command line
    if(argc!=4)
        error("Invalid number of args. Terminating.\n");
    portno = atoi(argv[2]);
    username = argv[3];

    // catch invalid ports
    if(portno<1024 || portno>65535)
        error("Invalid port. Terminating.\n");

    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Could not open socket. Terminating.\n");

    // get server host
    server = gethostbyname(argv[1]);
    if (server == NULL)
        error("Could not connect to server. Terminating.\n");

    // resolve address
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    // ======================
    // INITIALIZE CONNECTION:
    // ======================
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("Could not connect to server. Terminating.\n");

    // send username
    writeSocket(sockfd, 0x01, username);

    // recieve identifier and welcome message
    string identifier, message;
    string leftovers = "";
    char type = 0x00;
    readSocket(sockfd, type, identifier, leftovers);
    if(type != 0x02) // catch bad server init
        error("Invalid server initialization. Terminating.\n");
    readSocket(sockfd, type, message, leftovers);
    message = "Welcome Message: " + message;

    // print welcome message
    cout<<message;

    //===================
    // MAIN COMMAND LOOP:
    //===================
    int exit = 0, swInt = 0;
    string line = "";
    string dummy;
    type = 0x00;
    while(!exit && !cin.eof())
    {
        cout<<"Enter a command: (send, print, or exit)"<<endl;
        cin>>line;
        getline(cin, dummy);
        // translate command string into int for switch
        if(line == "send" || line == "Send" || line == "SEND") swInt = 1;
        else if(line == "print" || line == "Print" || line == "PRINT") swInt = 2;
        else if(line == "exit" || line == "Exit" || line == "EXIT") swInt = 3;
        else swInt = 0;
        switch(swInt)
        {
            // send POST command to the server with ID and message
            case 1:
                cout<<"Enter your message:"<<endl;
                getline(cin, line);
                writeSocket(sockfd, 0x04, identifier+"\n"+line);
                break;

            // send PRINTALL command to the server, then recieve/print full message board
            case 2:
                writeSocket(sockfd, 0x03, "");
                leftovers = "";
                readSocket(sockfd, type, line, leftovers);
                cout<<line<<endl;
                break;

            // send EXIT command to the server, then exit loop
            case 3:
                writeSocket(sockfd, 0x06, "");
                exit = 1;
                break;

            // invalid command case
            case 0:
            default:
                cout<<"Invalid command: "<<line<<endl;
                break;
        }
    }
    close(sockfd);
    return 0;
}

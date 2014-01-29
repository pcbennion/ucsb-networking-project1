/*
 * serverTCP.cpp
 *
 *  Created on: Oct 10, 2013
 *      Author: Peter
 */

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <vector>

using namespace std;

// Socket code and error messaging based on example found at:
//      http://www.linuxhowtos.org/C_C++/socket.htm

static int PACKSIZE = 128;

vector<string> dictionary;

string messageBoard;

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

/**
 * Create and register a user identifier ::STUB::
 */
string getID(string username)
{
    return username;
}

/**
 * Returns username if this identifier is registered ::STUB::
 */
string getName(string identifier)
{
    return identifier;
}

/**
 * Listen for and init connections
 */
int listenForConnections(int sockfd, string welcomeMsg)
{
    int newsockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr;

     string line;

    // listen for connections
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");

    // recieve username and create identifier
    string leftovers = "";
    char type = 0x00;
    readSocket(newsockfd, type, line, leftovers);
    string identifier = getID(line);

    // send identifier and welcome message
    writeSocket(newsockfd, 0x02, identifier);
    writeSocket(newsockfd, 0x05, welcomeMsg);

    return newsockfd;
}

/**
 * Loop where the server listens for client commands
 */
int primaryLoop(int newsockfd)
{
    int i, exit = 0;
    char type = 0x00;
    string line, username;
    string leftovers = "";
    stringstream ss;
    while(!exit)
    {
        // read incoming commands
        readSocket(newsockfd, type, line, leftovers);

        // switch on command byte
        switch(type)
        {
            // Send command: parse ID and post, add to board if valid ID
            case 0x04:
                ss<<line;
                i = line.find("\n");
                username = getName(line.substr(0, i));
                line = line.substr(i+1, string::npos) + "\n";
                if(username!="")
                    messageBoard = messageBoard + username + ": " + line + "\n";
                ss.str("");
                break;

            // Printall command: send messageBoard
            case 0x03:
                writeSocket(newsockfd, 0x05, messageBoard);
                break;

            // Exit command (or unrecognized): break loop
            case 0x06:
            default:
                exit = 1;
                break;
        }
    }

    // close connection
    close(newsockfd);

    return 0;
}

/**
 * Main function
 */
int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    struct sockaddr_in serv_addr;

    string welcomeMsg;

    int exit = 0;

    // grab port number from arguments, welcome message from cin
    if (argc < 2)
        error("Invalid number of args. Terminating.\n");
    portno = atoi(argv[1]);
    string line;
    welcomeMsg = "";
    while(!cin.eof())
    {
        getline(cin, line);
        welcomeMsg = welcomeMsg + line + "\n";
    }

    // catch invalid ports
    if(portno<1024 || portno>65535)
        error("Invalid port. Terminating.\n");

    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Could not open socket. Terminating.\n");

    // build address info
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // bind to socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("Could not bind port. Terminating.\n");

    // loop for accepting multiple non-concurrent connections
    // infinite loop - no safe server shutdown D:
    messageBoard = "";
    do  {
        newsockfd = listenForConnections(sockfd, welcomeMsg);
        exit = primaryLoop(newsockfd);
    } while (exit == 0);

    // close server socket and exit
    close(sockfd);
    return 0;

}



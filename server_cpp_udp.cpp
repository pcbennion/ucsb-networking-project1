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
#include <map>

using namespace std;

// Socket code and error messaging based on example found at:
//      http://www.linuxhowtos.org/C_C++/socket.htm

static int PACKSIZE = 128;

string messageBoard;

void error(const char *msg)
{
    fprintf(stderr, msg);
    exit(0);
}

/**
 * sends a packet with the given data. Waits for ACK.
 * tries three times before throwing error
 */
void sendPacket(int sock, string content, struct sockaddr* addr, socklen_t addrlen)
{
    int n;
    float t = 0.0f;
    char buffer[PACKSIZE];

    // try 3 times
    for(int tries = 0; tries < 3; tries++)
    {
        // send content
        //cout<<"Packet Contents:" <<content<<endl;
        n = sendto(sock, content.c_str(), content.size(), 0, addr, addrlen);
        if (n < 0) error("ERROR writing to socket");
        // wait for ACK
        //cout<<"\tWaiting for ACK"<<endl;
        while (t<10)
        {
            // check buffer for waiting packet
            bzero(buffer,PACKSIZE);
            recvfrom(sock, buffer, PACKSIZE-1, MSG_DONTWAIT, addr, &addrlen);
            if (n < 0) error("ERROR reading from socket");
            // exit with success state if packet is ACK packet
            if(string(buffer) == "!ACK!") return;
            t+=0.5f;
            sleep(0.5f);
        }
    }
    // if loop exits, then no ACK was recieved
    error("Failed to send message. Terminating.\n");
}

/**
 * sends a command through the socket, with a specified type and content
 */
void writeSocket(int sock, char type, string content, struct sockaddr* addr, socklen_t addrlen)
{
    string s, wrt;
    int numpackets, i;

    // put command bit in front of content
    s = type + content;

    // find number of packets to send
    // will always send one packet that has empty space.
    numpackets = s.size()/(PACKSIZE-4) + 1;

    char bytes[sizeof numpackets];

    // break content into appropriately sized packets.
    for(int seq = 0; seq < numpackets; seq++)
    {
        // create sequencing bytes. Based on answer at http://stackoverflow.com/questions/5585532/c-int-to-byte-array
        copy(static_cast<const char*>(static_cast<const void*>(&seq)),
              static_cast<const char*>(static_cast<const void*>(&seq)) + sizeof seq,
              bytes);

        // prepend first 4 sequence bytes to content
        string temp = bytes;
        for(i=temp.size(); i<4; i++)
            temp = temp + (char)0x00;
        s = temp + s;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        // send PACKSIZE bytes
        wrt = s.substr(0, PACKSIZE-1);
        //cout<<"Sending packet with content: "<<wrt<<"seq: "<<seq<<", and size:"<<wrt.size()<<endl;
        sendPacket(sock, wrt, addr, addrlen); // PROBLEM CODE!!!!!
        // above line somehow zeros out all elements of wrt when sent. debug line prior to it prints correct value
        // but the other side will recieve wrt.size() 0x00's
        ////////////////////////////////////////////////////////////////////////////////////////////////////

        // remove written part from content
        if(s.size()>PACKSIZE)
            s = s.substr(PACKSIZE, string::npos);
        else s = "";
    }
    cout<<endl;
}

/**
 * recieve a packet from the socket. Examine the packet and send an ACK
 * returns size of recieved packet
 */
int recvPacket(int sock, int seq, string& content, struct sockaddr* addr, socklen_t addrlen)
{
    int n = 0;
    char buffer[PACKSIZE];
    char bytes[4];
    float t = 0.0f;
    string temp;
    int flag;

    // get packet. First in sequence will block
    // otherwise, it will timeout if it takes too long
    if(seq <= 0) flag = 0;
    else flag = MSG_DONTWAIT;
    n = recvfrom(sock, buffer, PACKSIZE-1, flag, addr, &addrlen);
    if (n < 0) error("ERROR reading from socket");
    temp = buffer;
    while (n<=0)
    {
        t+=0.5f;
        sleep(0.5f);
        if(t>30) error("Failed to recieve message. Terminating.\n");
        n = recvfrom(sock, buffer, PACKSIZE-1, flag, addr, &addrlen);
        if (n < 0) error("ERROR reading from socket");
        temp = buffer;
    }

    // process sequencing data to decide if packet is the one we want
    if(n>3)
    {
        for(int i=0; i<4; i++)
            bytes[i] = buffer[i];
        n = (bytes[3]<<24)|(bytes[2]<<16)|(bytes[1]<<8)|(bytes[0]);

        // if this is not the packet we wanted, discard and wait for another
        if(n!=seq&&seq!=-1) return recvPacket(sock, seq, content, addr, addrlen);
    } else return recvPacket(sock, seq, content, addr, addrlen);

    cout<<temp<<endl;

    // send ACK
    n = sendto(sock, "!ACK!", 5, 0, addr, addrlen);
    if (n < 0) error("ERROR writing to socket");

    content = temp;
    n = content.size();
    content = content.substr(4, string::npos);

    return n;
}

/**
 * recieves and unpackages data from the socket
 */
void readSocket(int sock, char& type, string& content, struct sockaddr* addr, socklen_t addrlen)
{
    int i;
    string s = "";
    string line = "";

    // recieve packets until we get one with empty space
    i=0;
    do
    {
        recvPacket(sock, i, line, addr, addrlen);
        s = s+line;
        i++;
    } while(i>=PACKSIZE);

    // remove command byte
    type = s[0];
    s = s.substr(1, string::npos);

    content = s;
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
 * Listen for incoming packets, and decide what to do with them
 */
int listenForConnections(int sockfd, string welcomeMsg)
{
    socklen_t clilen;
    struct sockaddr cli_addr;

    int i, exit = 0;
    char type = 0x00;
    string line, username;
    stringstream ss;
    while(!exit)
    {
        // read incoming commands
        readSocket(sockfd, type, line, &cli_addr, clilen);

        // switch on command byte
        switch(type)
        {
            // Init command: send ID and welcome message
            case 0x01:
                username = getID(line);
                writeSocket(sockfd, 0x02, username, &cli_addr, clilen);
                writeSocket(sockfd, 0x05, welcomeMsg, &cli_addr, clilen);
                break;

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
                writeSocket(sockfd, 0x05, messageBoard, &cli_addr, clilen);
                break;

            // Exit command : no longer needs server-side action
            case 0x06:
                break;

            // Unknown command: close server
            default:
                exit = 1;
                break;
        }
    }
}

/**
 * Main function
 */
int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    struct sockaddr_in serv_addr;

    string welcomeMsg;

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
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

    // enter listening state
    messageBoard = "";
    newsockfd = listenForConnections(sockfd, welcomeMsg);
    close(sockfd);
    return 0;
}




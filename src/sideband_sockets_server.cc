//---------------------------------------------------------------------
// Windows server sockets performance test app
//---------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sys/types.h> 
#include <winsock2.h>
#include <sideband_data.h>

#pragma comment(lib, "Ws2_32.lib")

#define TEST_TCP_PORT 50055

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void WriteToSocket(int socketfd, void* buffer, int numBytes)
{
    auto remainingBytes = numBytes;
    while (remainingBytes > 0)
    {
        int written = send(socketfd, (const char*)buffer, remainingBytes, 0);
        if (written < 0)
        {
            std::cout << "Error writing to buffer";
            error(0);
        }
        remainingBytes -= written;
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void ReadFromSocket(int socket, void* buffer, int count)
{
    auto totalToRead = count;
    while (totalToRead > 0)
    {        
        int n;
        do
        {
            n = recv(socket, (char*)buffer, totalToRead, 0);
        } while (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK));

        if (n < 0)
        {
            std::cout << "Failed To read." << std::endl;
            error(0);
        }
        totalToRead -= n;
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int RunSidebandSocketsAccept()
{

    int sockfd, newsockfd;
    int clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    // create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
    {
       error("ERROR opening socket");
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    int portno = TEST_TCP_PORT;

    serv_addr.sin_family = AF_INET;  
    serv_addr.sin_addr.s_addr = INADDR_ANY;  
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
       error("ERROR on binding");
    }

    listen(sockfd, 5);

    while (true)
    {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
        { 
            error("ERROR on accept");
        }
        std::cout << "Connection!" << std::endl;

        char buffer[32];
        ReadFromSocket(newsockfd, buffer, 4);
        AddServerSidebandSocket(newsockfd, std::string(buffer, 4));
    }
    closesocket(newsockfd);
    closesocket(sockfd);
    return 0; 
}

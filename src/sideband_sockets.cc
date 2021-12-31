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
#include <sideband_internal.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sstream>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#pragma comment(lib, "Ws2_32.lib")

//---------------------------------------------------------------------
//---------------------------------------------------------------------
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
SOCKET ConnectTCPSocket(std::string address, std::string port, std::string usageId)
{    
    SOCKET connectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    struct addrinfo hints;
    int iResult;
    
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(address.c_str(), port.c_str(), &hints, &result);
    if ( iResult != 0 )
    {
        std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
        return 0;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL; ptr=ptr->ai_next)
    {
        // Create a SOCKET for connecting to server
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connectSocket == INVALID_SOCKET)
        {
            std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
            return 0;
        }

        // Connect to server.
        iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR)
        {
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }
    freeaddrinfo(result);
    if (connectSocket == INVALID_SOCKET)
    {
        std::cout << "Unable to connect to server!" << std::endl;
        return 0;
    }

    // Tell the server what shared memory location we are for
    WriteToSocket(connectSocket, const_cast<char*>(usageId.c_str()), usageId.length());
    return connectSocket;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketSidebandData::SocketSidebandData(uint64_t socket, const std::string& id) :
    _socket(socket),
    _id(id)
{
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketSidebandData::~SocketSidebandData()
{    
    closesocket(_socket);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
const std::string& SocketSidebandData::UsageId()
{
    return _id;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::vector<std::string> SplitUrlString(const std::string& s)
{
    char delimiter = ':';
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketSidebandData* SocketSidebandData::ClientInit(const std::string& sidebandServiceUrl, const std::string& usageId)
{    
    auto tokens = SplitUrlString(sidebandServiceUrl);
    auto socket = ConnectTCPSocket(tokens[0], tokens[1], usageId);
    auto sidebandData = new SocketSidebandData(socket, usageId);
    return sidebandData;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SocketSidebandData::Write(uint8_t* bytes, int bytecount)
{
    WriteToSocket(_socket, bytes, bytecount);    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SocketSidebandData::Read(uint8_t* bytes, int bufferSize, int* numBytesRead)
{
    ReadFromSocket(_socket, bytes, bufferSize);
    *numBytesRead = bufferSize;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int RunSidebandSocketsAccept()
{

    int sockfd, newsockfd;
    int clilen;
    struct sockaddr_in serv_addr, cli_addr;

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

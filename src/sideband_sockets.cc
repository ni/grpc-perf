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

#ifdef _WIN32
#include <winsock2.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#endif

#include <sideband_data.h>
#include <sideband_internal.h>
#include <sstream>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#define TEST_TCP_PORT 50055

#ifndef _WIN32
#define SOCKET int
#define INVALID_SOCKET 0
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool WriteToSocket(int socketfd, const void* buffer, int64_t numBytes)
{
    std::cout << "Writing To: " << socketfd << ", Start: " << *(int*)buffer << std::endl;
    auto remainingBytes = numBytes;
    const char* start = (const char*)buffer;
    while (remainingBytes > 0)
    {
        int written = send(socketfd, start, remainingBytes, 0);
        if (written < 0)
        {
            std::cout << "Error writing to buffer";
            return false;
        }
        start += written;
        remainingBytes -= written;
    }
    return true;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool ReadFromSocket(int socket, void* buffer, int count)
{
    auto totalToRead = count;
    char* start = (char*)buffer;
    while (totalToRead > 0)
    {        
        int n;
        do
        {
            n = recv(socket, start, totalToRead, 0);
        } while (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK));

        if (n < 0)
        {
            std::cout << "Failed To read." << std::endl;
            return false;
        }
        start += n;
        totalToRead -= n;
    }
    assert(totalToRead == 0);
    std::cout << "Read from: " << socket << ", Start: " << *(int*)buffer << std::endl;
    return true;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SOCKET ConnectTCPSocket(std::string address, std::string port, std::string usageId)
{
    std::cout << "Connect TCP Socket" << std::endl;

    SOCKET connectSocket = INVALID_SOCKET;
    
#ifdef _WIN32
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
        connectSocket = socket(ptr->ai_family, SOCK_STREAM, IPPROTO_TCP);
        if (connectSocket == INVALID_SOCKET)
        {
            std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
            return 0;
        }
        std::cout << "Socket Created" << std::endl;

        // Connect to server.
        iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR)
        {
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            continue;
        }
        std::cout << "Socket connected" << std::endl;
        break;
    }
    freeaddrinfo(result);
    if (connectSocket == INVALID_SOCKET)
    {
        std::cout << "Unable to connect to server!" << std::endl;
        return 0;
    }
#else
    int portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    portno = atoi(port.c_str());
    connectSocket = socket(AF_INET, SOCK_STREAM, 0);
    std::cout << "Socket created" << std::endl;
    if (connectSocket < 0) 
    {
        std::cout << "ERROR opening socket" << std::endl;
        return 0;
    }
    std::cout << "address: " << address << std::endl;
    server = gethostbyname(address.c_str());
    std::cout << "called get host by name" << std::endl;
    if (server == NULL)
    {
        std::cout << "ERROR, no such host" << std::endl;
        return 0;
    }
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(connectSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    { 
        std::cout << "ERROR connecting" << std::endl;
        return -1;
    }
    std::cout << "Socket connected" << std::endl;
#endif

    // Tell the server what shared memory location we are for
    WriteToSocket(connectSocket, const_cast<char*>(usageId.c_str()), usageId.length());
    return connectSocket;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketSidebandData::SocketSidebandData(uint64_t socket, const std::string& id, int64_t bufferSize) :
    SidebandData(bufferSize),
    _socket(socket),
    _id(id)
{
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketSidebandData::~SocketSidebandData()
{    
#ifdef _WIN32
    closesocket(_socket);
#else
    close(_socket);
#endif
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
    auto sidebandData = new SocketSidebandData(socket, usageId, 1024 * 1024);
    return sidebandData;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool SocketSidebandData::Write(const uint8_t* bytes, int64_t byteCount)
{
    return WriteToSocket(_socket, bytes, byteCount);    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool SocketSidebandData::Read(uint8_t* bytes, int64_t bufferSize, int64_t* numBytesRead)
{
    if (ReadFromSocket(_socket, bytes, bufferSize))
    {
        *numBytesRead = bufferSize;
        return true;
    }
    return false;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool SocketSidebandData::WriteLengthPrefixed(const uint8_t* bytes, int64_t byteCount)
{
    auto result = WriteToSocket(_socket, &byteCount, sizeof(int64_t));    
    if (result)
    {
        result = WriteToSocket(_socket, bytes, byteCount);    
    }
    return result;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool SocketSidebandData::ReadFromLengthPrefixed(uint8_t* bytes, int64_t bufferSize, int64_t* numBytesRead)
{
    return Read(bytes, bufferSize, numBytesRead);    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int64_t SocketSidebandData::ReadLengthPrefix()
{
    int64_t bufferSize = 0;
    ReadFromSocket(_socket, &bufferSize, sizeof(int64_t));
    return bufferSize;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::string GetSocketsAddress()
{
    return "10.0.0.53";
    //auto rdmaAddress = GetRdmaAddress();
    //if (rdmaAddress.length() > 0)
    //{
    //    return rdmaAddress;
    //}
    //return "localhost";
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int RunSidebandSocketsAccept()
{

    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr;

#ifdef _WIN32
    WSADATA wsaData {};
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

    // create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
    {
       std::cout << "ERROR opening socket" << std::endl;
       return -1;
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    int portno = TEST_TCP_PORT;

    serv_addr.sin_family = AF_INET;  
    serv_addr.sin_addr.s_addr = INADDR_ANY;  
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
       std::cout << "ERROR on binding" << std::endl;
       return -1;
    }

    listen(sockfd, 5);

    while (true)
    {
        sockaddr_in cli_addr {};
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
        { 
            std::cout << "ERROR on accept" << std::endl;
            return -1;
        }
        std::cout << "Connection!" << std::endl;

        char buffer[32];
        ReadFromSocket(newsockfd, buffer, 4);
        AddServerSidebandSocket(newsockfd, std::string(buffer, 4));
    }
#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
    return 0; 
}

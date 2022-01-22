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
bool WriteToSocket(int socket, const void* buffer, int64_t numBytes)
{
    auto remainingBytes = numBytes;
    const char* start = (const char*)buffer;
    while (remainingBytes > 0)
    {
        int written = send(socket, start, remainingBytes, 0);
        if (written < 0)
        {
            std::cout << "Error writing to buffer";
            return false;
        }
        start += written;
        remainingBytes -= written;
    }
    assert(remainingBytes == 0);
    return true;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool ReadFromSocket(int socket, void* buffer, int64_t numBytes)
{
    auto remainingBytes = numBytes;
    char* start = (char*)buffer;
    while (remainingBytes > 0)
    {        
        int n;
        do
        {
            n = recv(socket, start, remainingBytes, 0);
        } while (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK));

        if (n < 0)
        {
            std::cout << "Failed To read." << std::endl;
            return false;
        }
        start += n;
        remainingBytes -= n;
    }
    assert(remainingBytes == 0);
    return true;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SOCKET ConnectTCPSocket(std::string address, std::string port, std::string usageId)
{
    SOCKET connectSocket = INVALID_SOCKET;
    
#ifdef _WIN32
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* resultAddress = nullptr;
    auto result = getaddrinfo(address.c_str(), port.c_str(), &hints, &resultAddress);
    if (result != 0)
    {
        std::cout << "getaddrinfo failed with error: " << result << std::endl;
        return 0;
    }
    for (addrinfo* current = resultAddress; current != nullptr; current = current->ai_next)
    {
        connectSocket = socket(current->ai_family, SOCK_STREAM, IPPROTO_TCP);
        if (connectSocket == INVALID_SOCKET)
        {
            std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
            return 0;
        }
        result = connect(connectSocket, current->ai_addr, (int)current->ai_addrlen);
        if (result == SOCKET_ERROR)
        {
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }
    freeaddrinfo(resultAddress);
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
    if (connectSocket < 0) 
    {
        std::cout << "ERROR opening socket" << std::endl;
        return 0;
    }
    server = gethostbyname(address.c_str());
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
    auto rdmaAddress = GetRdmaAddress();
    if (rdmaAddress.length() > 0)
    {
        return rdmaAddress;
    }
    return "localhost";
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

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include "client_utilities.h"
#include "FastMemcpy.h"
#include <fstream>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "sideband_data.h"

class SidebandData;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#define BUF_SIZE 16 * 1024 * 1024
//TCHAR szName[] = TEXT("MyFileMappingObject");
TCHAR szMsg[] = TEXT("Message from first process.");

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::map<std::string, SidebandData*> _buffers;
bool _useFastMemcpy = false;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SetFastMemcpy(bool fastMemcpy)
{
    _useFastMemcpy = fastMemcpy;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class SidebandData
{
public:    
    virtual const std::string& UsageId() = 0;
    virtual void Write(uint8_t* bytes, int bytecount) = 0;
    virtual void Read(uint8_t* bytes, int bufferSize, int* numBytesRead) = 0;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class SharedMemorySidebandData : public SidebandData
{
public:
    SharedMemorySidebandData(const std::string& id);
    virtual ~SharedMemorySidebandData();

    void Write(uint8_t* bytes, int bytecount) override;
    void Read(uint8_t* bytes, int bufferSize, int* numBytesRead) override;

    const std::string& UsageId() override;
    uint8_t* GetBuffer();

private:
    void Init();

    HANDLE hMapFile;
    uint8_t* pBuf;
    std::string _id;
    std::string _usageId;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SharedMemorySidebandData::SharedMemorySidebandData(const std::string& id) :
    hMapFile(INVALID_HANDLE_VALUE),
    pBuf(nullptr),
    _usageId(id),
    _id("TESTBUFFER_" + id)
{
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SharedMemorySidebandData::~SharedMemorySidebandData()
{
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
const std::string& SharedMemorySidebandData::UsageId()
{    
    return _usageId;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
inline uint8_t* SharedMemorySidebandData::GetBuffer()
{
    if (pBuf == nullptr)
    {
        Init();
    }
    return pBuf;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SharedMemorySidebandData::Init()
{
    hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,    // use paging file
        NULL,                    // default security
        PAGE_READWRITE,          // read/write access
        0,                       // maximum object size (high-order DWORD)
        BUF_SIZE,                // maximum object size (low-order DWORD)
        _id.c_str());            // name of mapping object

    if (hMapFile == NULL)
    {
        std::cout << "Could not create file mapping object " << GetLastError() << std::endl;
        return;
    }
    pBuf = (uint8_t*)MapViewOfFile(hMapFile,   // handle to map object
        FILE_MAP_ALL_ACCESS, // read/write permission
        0,
        0,
        BUF_SIZE);

    if (pBuf == NULL)
    {
        std::cout << "Could not map view of file " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return;
    }
    return;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SharedMemorySidebandData::Write(uint8_t* bytes, int bytecount)
{    
    auto ptr = GetBuffer();
    if (_useFastMemcpy)
    {
        memcpy_fast(ptr, bytes, bytecount);
    }
    else
    {
        memcpy(ptr, bytes, bytecount);
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SharedMemorySidebandData::Read(uint8_t* bytes, int bufferSize, int* numBytesRead)
{    
    auto ptr = GetBuffer();
    if (_useFastMemcpy)
    {
        memcpy_fast(bytes, ptr, bufferSize);
    }
    else
    {
        memcpy(bytes, ptr, bufferSize);
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class SocketSidebandData : public SidebandData
{
public:
    SocketSidebandData(SOCKET socket, const std::string& id);
    virtual ~SocketSidebandData();

    void Write(uint8_t* bytes, int bytecount) override;
    void Read(uint8_t* bytes, int bufferSize, int* numBytesRead) override;

    const std::string& UsageId() override;

private:
    std::string _id;
    SOCKET _socket;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketSidebandData::SocketSidebandData(SOCKET socket, const std::string& id) :
    _socket(socket),
    _id(id)
{
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketSidebandData::~SocketSidebandData()
{    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
const std::string& SocketSidebandData::UsageId()
{
    return _id;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void WriteToSocket(SOCKET socketfd, void* buffer, int numBytes)
{
    auto remainingBytes = numBytes;
    while (remainingBytes > 0)
    {
        int written = send(socketfd, (char*)buffer, remainingBytes, 0);
        if (written < 0)
        {
            std::cout << "Error writing to buffer";            
        }
        remainingBytes -= written;
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void ReadFromSocket(SOCKET socket, void* buffer, int count)
{
    auto totalToRead = count;
    while (totalToRead > 0)
    {        
        int n = 0;
        do
        {
            n = recv(socket, (char*)buffer, totalToRead, 0);
        } while (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK));

        if (n < 0)
        {
            std::cout << "Failed To read." << std::endl;
        }
        totalToRead -= n;
    }
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
std::string InitOwnerSidebandData(::SidebandStrategy strategy, int64_t numSamples)
{
    if (strategy == ::SidebandStrategy::SOCKETS)
    {
        return "4444";
    }    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SOCKET ConnectTCPSocket(std::string address, std::string port, std::string usageId)
{    
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    int iResult;
    
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo("localhost", port.c_str(), &hints, &result);
    if ( iResult != 0 )
    {
        std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
        return 0;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next)
    {
        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET)
        {
            std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
            return 0;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR)
        {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }
    freeaddrinfo(result);
    if (ConnectSocket == INVALID_SOCKET)
    {
        std::cout << "Unable to connect to server!" << std::endl;
        return 0;
    }

    // Tell the server what shared memory location we are for
    WriteToSocket(ConnectSocket, const_cast<char*>(usageId.c_str()), usageId.length());
    return ConnectSocket;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int64_t GetOwnerSidebandDataToken(const std::string& usageId)
{
    return reinterpret_cast<int64_t>(_buffers[usageId]);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::vector<std::string> split(const std::string& s, char delimiter)
{
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
int64_t InitClientSidebandData(const std::string& sidebandServiceUrl, ::SidebandStrategy strategy, const std::string& usageId)
{
    if (strategy == ::SidebandStrategy::SOCKETS)
    {
        auto tokens = split(sidebandServiceUrl, ':');
        auto socket = ConnectTCPSocket(tokens[0], tokens[1], usageId);
        auto sidebandData = new SocketSidebandData(socket, usageId);
        _buffers.emplace(usageId, sidebandData);
        return reinterpret_cast<int64_t>(sidebandData);
    }
    return 0;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void AddServerSidebandSocket(int socket, const std::string& usageId)
{    
    auto sidebandData = new SocketSidebandData(socket, usageId);
    _buffers.emplace(usageId, sidebandData);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void WriteSidebandData(int64_t dataToken, uint8_t* bytes, int bytecount)
{
    auto sidebandData = reinterpret_cast<SidebandData*>(dataToken);
    sidebandData->Write(bytes, bytecount);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void ReadSidebandData(int64_t dataToken, uint8_t* bytes, int bufferSize, int* numBytesRead)
{    
    auto sidebandData = reinterpret_cast<SidebandData*>(dataToken);
    sidebandData->Read(bytes, bufferSize, numBytesRead);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void CloseSidebandData(int64_t dataToken)
{    
}

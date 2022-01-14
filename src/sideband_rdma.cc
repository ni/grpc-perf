//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <iostream>
#include "ni-rdma.h"
#include <vector>
#include <thread>
#include "Semaphore.h"
#include <algorithm>
#include <thread>
#include <future>
#include <cassert>
#include <sideband_data.h>
#include <sideband_internal.h>
#include <Semaphore.h>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int32_t timeoutMs = -1;
int MaxConcurrentTransactions = 1;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class RdmaSidebandDataImp
{
public:
    RdmaSidebandDataImp(nirdma_Session connectedWriteSession, nirdma_Session connectedReadSession, bool lowLatency);
    ~RdmaSidebandDataImp();

    void Write(const uint8_t* bytes, int64_t bytecount);
    void Read(uint8_t* bytes, int64_t bufferSize, int64_t* numBytesRead);
    void ReadFromLengthPrefixed(uint8_t* bytes, int64_t bufferSize, int64_t* numBytesRead);
    int64_t ReadLengthPrefix();

    const uint8_t* BeginDirectRead(int64_t byteCount);
    const uint8_t* BeginDirectReadLengthPrefixed(int64_t* bufferSize);
    bool FinishDirectRead();
    uint8_t* BeginDirectWrite();
    bool FinishDirectWrite(int64_t byteCount);

    static void QueueSidebandConnection(::SidebandStrategy strategy, bool waitForReader, bool waitForWriter, int64_t bufferSize);
    static RdmaSidebandData* InitFromConnection(nirdma_Session connectedSession, bool isWriteSession);
    static RdmaSidebandData* ClientInitFromConnection(nirdma_Session connectedWriteSession, nirdma_Session connectedReadSession, bool lowLatency);
    static void ServerReceiveData(void* context1, void* context2, int32_t completionStatus, size_t completedBytes);

private:
    bool _lowLatency;
    nirdma_Session _connectedWriteSession;
    nirdma_Session _connectedReadSession;

    std::vector<uint8_t> _buffer;
    int64_t _bufferSize;
    Semaphore _receiveSemaphore;

private:    
    static Semaphore _rdmaConnectQueue;
    static nirdma_Session _pendingWriteSession;
    static nirdma_Session _pendingReadSession;
    static bool _nextConnectLowLatency;
    static bool _waitForReaderConnection;
    static bool _waitForWriteConnection;
    static int64_t _nextConnectBufferSize;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
RdmaSidebandData::RdmaSidebandData(const std::string& id, RdmaSidebandDataImp* implementation) :
    _id(id),
    _imp(implementation)
{    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
RdmaSidebandData::~RdmaSidebandData()
{    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
RdmaSidebandData* RdmaSidebandData::ClientInit(const std::string& sidebandServiceUrl, bool lowLatency, const std::string& usageId, int64_t bufferSize)
{
    auto localAddress = GetRdmaAddress();
    auto tokens = SplitUrlString(sidebandServiceUrl);

    std::cout << "Client connetion using local address: " << localAddress << std::endl;

    nirdma_Session clientReadSession = nirdma_InvalidSession;
    auto result = nirdma_CreateConnectorSession(localAddress.c_str(), 0, &clientReadSession);   
    result = nirdma_Connect(clientReadSession, nirdma_Direction_Receive, tokens[0].c_str(), std::stoi(tokens[1]), timeoutMs);
    if (result != nirdma_Error_Success)
    {
        std::cout << "Failed to connect: " << result << std::endl;
    }
    assert(result == nirdma_Error_Success);

    nirdma_Session clientWriteSession = nirdma_InvalidSession;
    result = nirdma_CreateConnectorSession(localAddress.c_str(), 0, &clientReadSession);   
    result = nirdma_Connect(clientReadSession, nirdma_Direction_Send, tokens[0].c_str(), std::stoi(tokens[1])+1, timeoutMs);
    if (result != nirdma_Error_Success)
    {
        std::cout << "Failed to connect: " << result << std::endl;
    }
    assert(result == nirdma_Error_Success);

    auto sidebandData = RdmaSidebandDataImp::ClientInitFromConnection(clientWriteSession, clientReadSession, lowLatency);
    return sidebandData;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RdmaSidebandData::Write(const uint8_t* bytes, int64_t bytecount)
{
    _imp->Write(bytes, bytecount);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RdmaSidebandData::Read(uint8_t* bytes, int64_t bufferSize, int64_t* numBytesRead)
{
    _imp->Read(bytes, bufferSize, numBytesRead);    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RdmaSidebandData::ReadFromLengthPrefixed(uint8_t* bytes, int64_t bufferSize, int64_t* numBytesRead)
{
    _imp->ReadFromLengthPrefixed(bytes, bufferSize, numBytesRead);    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int64_t RdmaSidebandData::ReadLengthPrefix()
{
    return _imp->ReadLengthPrefix();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
const uint8_t* RdmaSidebandData::BeginDirectRead(int64_t byteCount)
{
    return _imp->BeginDirectRead(byteCount);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
const uint8_t* RdmaSidebandData::BeginDirectReadLengthPrefixed(int64_t* bufferSize)
{
    return _imp->BeginDirectReadLengthPrefixed(bufferSize);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool RdmaSidebandData::FinishDirectRead()
{
    return _imp->FinishDirectRead();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
uint8_t* RdmaSidebandData::BeginDirectWrite()
{
    return _imp->BeginDirectWrite();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool RdmaSidebandData::FinishDirectWrite(int64_t byteCount)
{
    return _imp->FinishDirectWrite(byteCount);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
const std::string& RdmaSidebandData::UsageId()
{
    return _id;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Semaphore RdmaSidebandDataImp::_rdmaConnectQueue;
nirdma_Session RdmaSidebandDataImp::_pendingWriteSession;
nirdma_Session RdmaSidebandDataImp::_pendingReadSession;
bool RdmaSidebandDataImp::_nextConnectLowLatency;
bool RdmaSidebandDataImp::_waitForReaderConnection;
bool RdmaSidebandDataImp::_waitForWriteConnection;
int64_t RdmaSidebandDataImp::_nextConnectBufferSize;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
RdmaSidebandDataImp::RdmaSidebandDataImp(nirdma_Session connectedWriteSession, nirdma_Session connectedReadSession, bool lowLatency) :
    _connectedWriteSession(connectedWriteSession),
    _connectedReadSession(connectedReadSession),
    _lowLatency(lowLatency)
{
    _bufferSize = 4 * 1024 * 1024;
    _buffer.reserve(_bufferSize);

    auto result = nirdma_ConfigureExternalBuffer(_connectedWriteSession, _buffer.data(), _bufferSize, MaxConcurrentTransactions);
    if (result != 0)
    {
        std::cout << "Failed nirdma_ConfigureExternalBuffer: " << result << std::endl;
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
RdmaSidebandDataImp::~RdmaSidebandDataImp()
{    
    if (_connectedWriteSession != nirdma_InvalidSession)
    {
        auto result = nirdma_CloseSession(_connectedWriteSession);
        if (result != 0)
        {
            std::cout << "Failed nirdma_CloseSession connected write session: " << result << std::endl;
        }
    }
    if (_connectedReadSession != nirdma_InvalidSession)
    {
        auto result = nirdma_CloseSession(_connectedReadSession);
        if (result != 0)
        {
            std::cout << "Failed nirdma_CloseSession connected read session: " << result << std::endl;
        }
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RdmaSidebandDataImp::QueueSidebandConnection(::SidebandStrategy strategy, bool waitForReader, bool waitForWriter, int64_t bufferSize)
{
    _rdmaConnectQueue.wait();
    _pendingWriteSession = nirdma_InvalidSession;
    _pendingReadSession = nirdma_InvalidSession;
    _nextConnectLowLatency = true;
    _nextConnectBufferSize = bufferSize;
    _waitForReaderConnection = waitForReader;
    _waitForWriteConnection = waitForWriter;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
RdmaSidebandData* RdmaSidebandDataImp::ClientInitFromConnection(nirdma_Session connectedWriteSession, nirdma_Session connectedReadSession, bool lowLatency)
{    
    auto imp = new RdmaSidebandDataImp(connectedWriteSession, connectedReadSession, lowLatency);
    auto sidebandData = new RdmaSidebandData("TEST_RDMA", imp);
    RegisterSidebandData(sidebandData);
    return sidebandData;        
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
RdmaSidebandData* RdmaSidebandDataImp::InitFromConnection(nirdma_Session connectedSession, bool isWriteSession)
{
    if (isWriteSession)
    {
        _pendingWriteSession = connectedSession;
    }
    else
    {
        _pendingReadSession = connectedSession;
    }
    if ((_pendingWriteSession != nirdma_InvalidSession || !_waitForWriteConnection) &&
        (_pendingReadSession != nirdma_InvalidSession || !_waitForReaderConnection))
    {
        auto imp = new RdmaSidebandDataImp(_pendingWriteSession, _pendingReadSession, _nextConnectLowLatency);
        auto sidebandData = new RdmaSidebandData("TEST_RDMA", imp);
        RegisterSidebandData(sidebandData);
        return sidebandData;        
    }
    return nullptr;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RdmaSidebandDataImp::ServerReceiveData(void* context1, void* context2, int32_t completionStatus, size_t completedBytes)
{
    ((RdmaSidebandDataImp*)(context1))->_receiveSemaphore.notify();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RdmaSidebandDataImp::Write(const uint8_t* bytes, int64_t bytecount)
{    
    auto result = nirdma_QueueExternalBufferRegion(_connectedWriteSession, _buffer.data(), _bufferSize, nullptr, timeoutMs);
    if (result != 0)
    {
        std::cout << "Failed nirdma_QueueExternalBufferRegion: " << result << std::endl;
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RdmaSidebandDataImp::Read(uint8_t* bytes, int64_t bufferSize, int64_t* numBytesRead)
{    
    nirdma_BufferCompletionCallbackData bufferReady;
    bufferReady.callbackFunction = RdmaSidebandDataImp::ServerReceiveData;
    bufferReady.context1 = this;
    bufferReady.context2 = nullptr;

    auto result = nirdma_QueueExternalBufferRegion(_connectedReadSession, _buffer.data(), _bufferSize, &bufferReady, timeoutMs);
    if (result != 0)
    {
        std::cout << "Failed nirdma_QueueExternalBufferRegion: " << result << std::endl;
    }
    _receiveSemaphore.wait();
    if (bytes != nullptr)
    {
        memcpy(bytes, _buffer.data(), bufferSize);
        *numBytesRead = bufferSize;
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RdmaSidebandDataImp::ReadFromLengthPrefixed(uint8_t* bytes, int64_t bufferSize, int64_t* numBytesRead)
{    
    memcpy(bytes, _buffer.data(), bufferSize);
    *numBytesRead = bufferSize;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int64_t RdmaSidebandDataImp::ReadLengthPrefix()
{    
    Read(nullptr, 0, nullptr);
    return *reinterpret_cast<int64_t*>(_buffer.data());
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
const uint8_t* RdmaSidebandDataImp::BeginDirectRead(int64_t byteCount)
{    
    Read(nullptr, 0, nullptr);
    return _buffer.data();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
const uint8_t* RdmaSidebandDataImp::BeginDirectReadLengthPrefixed(int64_t* bufferSize)
{
    Read(nullptr, 0, nullptr);
    *bufferSize = *reinterpret_cast<int64_t*>(_buffer.data());
    return _buffer.data() + sizeof(int64_t);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool RdmaSidebandDataImp::FinishDirectRead()
{
    return true;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
uint8_t* RdmaSidebandDataImp::BeginDirectWrite()
{
    return _buffer.data();    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool RdmaSidebandDataImp::FinishDirectWrite(int64_t byteCount)
{
    auto result = nirdma_QueueExternalBufferRegion(_connectedWriteSession, _buffer.data(), _bufferSize, nullptr, timeoutMs);
    if (result != 0)
    {
        std::cout << "Failed nirdma_QueueExternalBufferRegion: " << result << std::endl;
    }
    return true;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::string GetRdmaAddress()
{    
    size_t numAddresses = 0;

    auto result = nirdma_Enumerate(nullptr, &numAddresses, nirdma_AddressFamily_AF_INET);
    std::vector<std::string> interfaces;
    if (numAddresses != 0)
    {
        std::vector<nirdma_AddressString> addresses(numAddresses);
        auto result2 = nirdma_Enumerate(&addresses[0], &numAddresses, nirdma_AddressFamily_AF_INET);
        for (auto addr : addresses)
        {
            interfaces.push_back(&addr.addressString[0]);
        }
    }
    assert(interfaces.size() == 1);
    return interfaces.front();    
}


//---------------------------------------------------------------------
//---------------------------------------------------------------------
void QueueSidebandConnection(::SidebandStrategy strategy, bool waitForReader, bool waitForWriter, int64_t bufferSize)
{
    bool lowLatency = false;
    switch (strategy)
    {
        case ::SidebandStrategy::RDMA:
            break;
        case ::SidebandStrategy::RDMA_LOW_LATENCY:
            lowLatency = true;
            break;
        default:
            // don't need to queue for non RDMA strategies
            return;
    }
    RdmaSidebandDataImp::QueueSidebandConnection(strategy, waitForReader, waitForWriter, bufferSize);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int AcceptSidebandRdmaRequests(int direction, int port)
{
    std::string listenAddress = GetRdmaAddress();
    std::cout << "Listening for RDMA at: " << listenAddress << ":" << port << std::endl;

    nirdma_Session listenSession = nirdma_InvalidSession;
    auto result = nirdma_CreateListenerSession(listenAddress.c_str(), port, &listenSession);
    if (result != 0)
    {
        std::cout << "Failed nirdma_CreateListenerSession: " << result << std::endl;
    }

    while (true)
    {
        nirdma_Session connectedSession = nirdma_InvalidSession;
        auto r = nirdma_Accept(listenSession, direction, timeoutMs, &connectedSession);
        if (r != 0)
        {
            std::cout << "Failed nirdma_CreateListenerSession: " << r << std::endl;
        }
        assert(r == 0);
        std::cout << "RDMA Connection!" << std::endl;        
        RdmaSidebandDataImp::InitFromConnection(connectedSession, direction == nirdma_Direction_Send);
    }
    return 0;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int AcceptSidebandRdmaSendRequests()
{    
    int port = 50060;
    return AcceptSidebandRdmaRequests(nirdma_Direction_Send, port);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int AcceptSidebandRdmaReceiveRequests()
{    
    int port = 50061;
    return AcceptSidebandRdmaRequests(nirdma_Direction_Receive, port);
}

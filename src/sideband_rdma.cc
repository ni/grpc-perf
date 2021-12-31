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
#pragma comment(lib, "D:/dev/chrisc-grpc-perf/src/rdma.lib")

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int32_t timeoutMs = -1;
int MaxConcurrentTransactions = 1;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class RdmaSidebandDataImp
{
public:
    RdmaSidebandDataImp(nirdma_Session connectedSession);
    ~RdmaSidebandDataImp();

    void Write(uint8_t* bytes, int bytecount);
    void Read(uint8_t* bytes, int bufferSize, int* numBytesRead);

    static RdmaSidebandData* InitFromConnection(nirdma_Session connectedSession);
    static void ServerReceiveData(void* context1, void* context2, int32_t completionStatus, size_t completedBytes);

private:
    nirdma_Session _connectedSession;
    std::vector<uint8_t> _buffer;
    int64_t _bufferSize;
    Semaphore _receiveSemaphore;
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
RdmaSidebandData* RdmaSidebandData::ClientInit(const std::string& sidebandServiceUrl, const std::string& usageId, int64_t bufferSize)
{
    const char* localAddress = "169.254.171.14";

    nirdma_Session clientSession = nirdma_InvalidSession;
    auto result = nirdma_CreateConnectorSession(localAddress, 0, &clientSession);
    
    auto tokens = SplitUrlString(sidebandServiceUrl);
    result = nirdma_Connect(clientSession, nirdma_Direction_Receive, tokens[0].c_str(), std::stoi(tokens[1]), timeoutMs);
    auto sidebandData = RdmaSidebandDataImp::InitFromConnection(clientSession);
    return sidebandData;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RdmaSidebandData::Write(uint8_t* bytes, int bytecount)
{
    _imp->Write(bytes, bytecount);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RdmaSidebandData::Read(uint8_t* bytes, int bufferSize, int* numBytesRead)
{
    _imp->Read(bytes, bufferSize, numBytesRead);    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
const std::string& RdmaSidebandData::UsageId()
{
    return _id;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
RdmaSidebandDataImp::RdmaSidebandDataImp(nirdma_Session connectedSession) :
    _connectedSession(connectedSession)
{
    _bufferSize = 4 * 1024 * 1024;
    _buffer.reserve(_bufferSize);    

    auto result = nirdma_ConfigureExternalBuffer(_connectedSession, _buffer.data(), _bufferSize, MaxConcurrentTransactions);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
RdmaSidebandDataImp::~RdmaSidebandDataImp()
{    
    auto result = nirdma_CloseSession(_connectedSession);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
RdmaSidebandData* RdmaSidebandDataImp::InitFromConnection(nirdma_Session connectedSession)
{
    auto imp = new RdmaSidebandDataImp(connectedSession);
    auto sidebandData = new RdmaSidebandData("TEST_RDMA", imp);
    RegisterSidebandData(sidebandData);
    return sidebandData;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RdmaSidebandDataImp::ServerReceiveData(void* context1, void* context2, int32_t completionStatus, size_t completedBytes)
{
    ((RdmaSidebandDataImp*)(context1))->_receiveSemaphore.notify();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RdmaSidebandDataImp::Write(uint8_t* bytes, int bytecount)
{    
    auto result = nirdma_QueueExternalBufferRegion(_connectedSession, _buffer.data(), _bufferSize, nullptr, timeoutMs);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RdmaSidebandDataImp::Read(uint8_t* bytes, int bufferSize, int* numBytesRead)
{    
    nirdma_BufferCompletionCallbackData bufferReady;
    bufferReady.callbackFunction = RdmaSidebandDataImp::ServerReceiveData;
    bufferReady.context1 = this;
    bufferReady.context2 = nullptr;

    auto result = nirdma_QueueExternalBufferRegion(_connectedSession, _buffer.data(), _bufferSize, &bufferReady, timeoutMs);
    _receiveSemaphore.wait();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int AcceptSidebandRdmaRequests()
{
    std::string listenAddress = "169.254.179.151";
    int port = 50060;

    nirdma_Session listenSession = nirdma_InvalidSession;
    auto result = nirdma_CreateListenerSession(listenAddress.c_str(), port, &listenSession);

    while (true)
    {
        nirdma_Session connectedSession = nirdma_InvalidSession;
        auto r = nirdma_Accept(listenSession, nirdma_Direction_Send, timeoutMs, &connectedSession);
        assert(r == 0);

        RdmaSidebandDataImp::InitFromConnection(connectedSession);
    }
    return 0;
}
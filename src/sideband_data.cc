//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include "client_utilities.h"
#include <fstream>
#include "sideband_data.h"
#include "sideband_internal.h"

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::mutex _bufferLockMutex;
std::condition_variable _bufferLock;
std::map<std::string, SidebandData*> _buffers;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SidebandData::~SidebandData()
{
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::string InitOwnerSidebandData(::SidebandStrategy strategy, int64_t bufferSize)
{
    std::unique_lock<std::mutex> lock(_bufferLockMutex);
    switch (strategy)
    {
        case ::SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY:
            {
                auto sidebandData = DoubleBufferedSharedMemorySidebandData::InitNew(bufferSize);
                _buffers.emplace(sidebandData->UsageId(), sidebandData);
                return sidebandData->UsageId();
            }
            break;
        case ::SidebandStrategy::SHARED_MEMORY:
            {
                auto sidebandData = SharedMemorySidebandData::InitNew(bufferSize);
                _buffers.emplace(sidebandData->UsageId(), sidebandData);
                return sidebandData->UsageId();
            }
            break;
        case ::SidebandStrategy::SOCKETS:
            return "4444";
        case ::SidebandStrategy::RDMA:
            return "TEST_RDMA";
    }
    assert(false);
    return std::string();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int64_t GetOwnerSidebandDataToken(const std::string& usageId)
{
    std::unique_lock<std::mutex> lock(_bufferLockMutex);
    
    while (_buffers.find(usageId) == _buffers.end()) _bufferLock.wait(lock);
    return reinterpret_cast<int64_t>(_buffers[usageId]);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int64_t InitClientSidebandData(const std::string& sidebandServiceUrl, ::SidebandStrategy strategy, const std::string& usageId, int bufferSize)
{
    SidebandData* sidebandData = nullptr;
    bool insert = true;
    switch (strategy)
    {
        case ::SidebandStrategy::SHARED_MEMORY:
            sidebandData = new SharedMemorySidebandData(usageId, bufferSize);
            break;
        case ::SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY:
            sidebandData = new DoubleBufferedSharedMemorySidebandData(usageId, bufferSize);
            break;
        case ::SidebandStrategy::SOCKETS:
            sidebandData = SocketSidebandData::ClientInit(sidebandServiceUrl, usageId);
            break;
        case ::SidebandStrategy::RDMA:
            sidebandData = RdmaSidebandData::ClientInit(sidebandServiceUrl, usageId, bufferSize);
            insert = false;
            break;
    }
    if (insert)
    {
        std::unique_lock<std::mutex> lock(_bufferLockMutex);

        assert(_buffers.find(usageId) == _buffers.end());
        _buffers.emplace(usageId, sidebandData);
    }
    return reinterpret_cast<int64_t>(sidebandData);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void AddServerSidebandSocket(int socket, const std::string& usageId)
{    
    std::unique_lock<std::mutex> lock(_bufferLockMutex);

    auto sidebandData = new SocketSidebandData(socket, usageId);
    assert(_buffers.find(sidebandData->UsageId()) == _buffers.end());
    _buffers.emplace(usageId, sidebandData);

    _bufferLock.notify_all();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RegisterSidebandData(SidebandData* sidebandData)
{    
    std::unique_lock<std::mutex> lock(_bufferLockMutex);

    assert(_buffers.find(sidebandData->UsageId()) == _buffers.end());
    _buffers.emplace(sidebandData->UsageId(), sidebandData);

    _bufferLock.notify_all();
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
    std::unique_lock<std::mutex> lock(_bufferLockMutex);

    auto sidebandData = reinterpret_cast<SidebandData*>(dataToken);
    assert(_buffers.find(sidebandData->UsageId()) != _buffers.end());
    _buffers.erase(sidebandData->UsageId());
    delete sidebandData;
}

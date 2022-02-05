//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include "client_utilities.h"
#include <fstream>
#include "sideband_data.h"
#include "sideband_internal.h"
#include <condition_variable>
#include <atomic>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::mutex _bufferLockMutex;
std::condition_variable _bufferLock;
std::map<std::string, SidebandData*> _buffers;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SidebandData::SidebandData(int64_t bufferSize) :
    _bufferSize(bufferSize)
{        
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SidebandData::~SidebandData()
{
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
uint8_t* SidebandData::SerializeBuffer()
{
    if (_serializeBuffer.size() == 0)
    {
        _serializeBuffer = std::vector<uint8_t>(_bufferSize);
    }
    return _serializeBuffer.data();
}


std::atomic_int _nextId;
std::string _zeroId = NextConnectionId();

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int ConnectIdLength()
{
    return _zeroId.length();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::string NextConnectionId()
{
    char buffer[20];
    auto id = ++_nextId;
    sprintf_s(buffer, "ID:%10d", id);
    return buffer;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void QueueSidebandConnection(::SidebandStrategy strategy, const std::string& id, bool waitForReader, bool waitForWriter, int64_t bufferSize)
{
    switch (strategy)
    {
    case ::SidebandStrategy::RDMA:
    case ::SidebandStrategy::RDMA_LOW_LATENCY:
        RdmaSidebandData::QueueSidebandConnection(strategy, id, waitForReader, waitForWriter, bufferSize);
        break;
    case ::SidebandStrategy::SOCKETS:
    case ::SidebandStrategy::SOCKETS_LOW_LATENCY:
        SocketSidebandData::QueueSidebandConnection(strategy, id, bufferSize);
    default:
        // don't need to queue for non RDMA strategies
        return;
    }
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
        case ::SidebandStrategy::SOCKETS_LOW_LATENCY:
            return NextConnectionId();
        case ::SidebandStrategy::RDMA:
        case ::SidebandStrategy::RDMA_LOW_LATENCY:
            return NextConnectionId();
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
        case ::SidebandStrategy::SOCKETS_LOW_LATENCY:
            sidebandData = SocketSidebandData::ClientInit(sidebandServiceUrl, usageId, bufferSize, strategy == ::SidebandStrategy::SOCKETS_LOW_LATENCY);
            break;
        case ::SidebandStrategy::RDMA:
            sidebandData = RdmaSidebandData::ClientInit(sidebandServiceUrl, false, usageId, bufferSize);
            insert = false;
            break;
        case ::SidebandStrategy::RDMA_LOW_LATENCY:
            sidebandData = RdmaSidebandData::ClientInit(sidebandServiceUrl, true, usageId, bufferSize);
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
int64_t InitClientSidebandData(const BeginMonikerSidebandStreamResponse& response)
{
    return InitClientSidebandData(response.connection_url(), (::SidebandStrategy)response.strategy(), response.sideband_identifier(), response.buffer_size());    
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
void WriteSidebandData(int64_t dataToken, uint8_t* bytes, int64_t bytecount)
{
    auto sidebandData = reinterpret_cast<SidebandData*>(dataToken);
    sidebandData->Write(bytes, bytecount);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void ReadSidebandData(int64_t dataToken, uint8_t* bytes, int64_t bufferSize, int64_t* numBytesRead)
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

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::string GetConnectionAddress(::SidebandStrategy strategy)
{
    std::string address;
    if (strategy == ::SidebandStrategy::RDMA || 
        strategy == ::SidebandStrategy::RDMA_LOW_LATENCY)
    {
        address = GetRdmaAddress() + ":50060";
    }
    else
    {
        address = GetSocketsAddress() + ":50055";
    }
    std::cout << "Connection address: " << address << std::endl;
    return address;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool ReadSidebandMessage(int64_t dataToken, google::protobuf::MessageLite* message)
{    
    auto sidebandData = reinterpret_cast<SidebandData*>(dataToken);
    bool success = false;
    if (sidebandData->SupportsDirectReadWrite())
    {
        int64_t bufferSize = 0;
        auto buffer = sidebandData->BeginDirectReadLengthPrefixed(&bufferSize);
        success = message->ParseFromArray(buffer, bufferSize);
        sidebandData->FinishDirectRead();
    }
    else
    {
        auto bufferSize = sidebandData->ReadLengthPrefix();
        int64_t bytesRead = 0;
        sidebandData->ReadFromLengthPrefixed(sidebandData->SerializeBuffer(), bufferSize, &bytesRead);
        success = message->ParseFromArray(sidebandData->SerializeBuffer(), bufferSize);
        assert(success);
    }
    return success;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int64_t InitMonikerSidebandData(const BeginMonikerSidebandStreamResponse& initResponse)
{
    return InitClientSidebandData(initResponse.connection_url(), (::SidebandStrategy)initResponse.strategy(), initResponse.sideband_identifier(), initResponse.buffer_size());
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int64_t WriteSidebandMessage(int64_t dataToken, const google::protobuf::MessageLite& message)
{
    auto sidebandData = reinterpret_cast<SidebandData*>(dataToken);
    auto byteSize = message.ByteSizeLong();
    if (sidebandData->SupportsDirectReadWrite())
    {
        auto buffer = sidebandData->BeginDirectWrite();
        message.SerializeToArray(buffer, byteSize);
        sidebandData->FinishDirectWrite(byteSize);
    }
    else
    {
        message.SerializeToArray(sidebandData->SerializeBuffer(), byteSize);
        sidebandData->WriteLengthPrefixed(sidebandData->SerializeBuffer(), byteSize);
    }
    return byteSize;
}

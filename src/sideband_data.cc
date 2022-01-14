//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include "client_utilities.h"
#include <fstream>
#include "sideband_data.h"
#include "sideband_internal.h"
#include <condition_variable>

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
        case ::SidebandStrategy::RDMA_LOW_LATENCY:
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
    if (strategy == ::SidebandStrategy::RDMA)
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
        if (bufferSize > sidebandData->_serializeBuffer.size())
        {
            sidebandData->_serializeBuffer.reserve(bufferSize);
        }
        int64_t bytesRead = 0;
        sidebandData->ReadFromLengthPrefixed(sidebandData->_serializeBuffer.data(), bufferSize, &bytesRead);
        success = message->ParseFromArray(sidebandData->_serializeBuffer.data(), bytesRead);
    }
    return success;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool ReadSidebandMonikerRequest(int64_t dataToken, MonikerWriteRequest* writeRequest)
{
    return ReadSidebandMessage(dataToken, writeRequest);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool ReadSidebandMonikerResponse(int64_t dataToken, MonikerReadResponse* response)
{    
    return ReadSidebandMessage(dataToken, response);
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
    auto byteSize = message.ByteSizeLong() + sizeof(int64_t);
    if (sidebandData->SupportsDirectReadWrite())
    {
        auto buffer = sidebandData->BeginDirectWrite();
        message.SerializeToArray(buffer + sizeof(int64_t), byteSize - sizeof(int64_t));
        *(reinterpret_cast<int64_t*>(buffer)) = byteSize;
        sidebandData->FinishDirectWrite(byteSize);
    }
    else
    {
        message.SerializeToArray(sidebandData->_serializeBuffer.data(), byteSize - sizeof(int64_t));
        sidebandData->Write(sidebandData->_serializeBuffer.data(), byteSize);
    }
    return byteSize;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int64_t WriteSidebandMonikers(int64_t dataToken, const MonikerWriteRequest& writeRequest)
{
    return WriteSidebandMessage(dataToken, writeRequest);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int64_t WriteSidebandMonikers(int64_t dataToken, const MonikerReadResponse& response)
{
    return WriteSidebandMessage(dataToken, response);
}

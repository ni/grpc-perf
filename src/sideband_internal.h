//---------------------------------------------------------------------
//---------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <client_utilities.h>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class SidebandData
{
public:    
    virtual ~SidebandData();
    virtual const std::string& UsageId() = 0;
    virtual void Write(uint8_t* bytes, int bytecount) = 0;
    virtual void Read(uint8_t* bytes, int bufferSize, int* numBytesRead) = 0;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class SharedMemorySidebandData : public SidebandData
{
public:
    SharedMemorySidebandData(const std::string& id, int64_t bufferSize);
    virtual ~SharedMemorySidebandData();

    void Write(uint8_t* bytes, int bytecount) override;
    void Read(uint8_t* bytes, int bufferSize, int* numBytesRead) override;

    const std::string& UsageId() override;
    uint8_t* GetBuffer();

public:
    static SharedMemorySidebandData* InitNew(int64_t bufferSize);

private:
    void Init();
#ifdef _WIN32
    HANDLE _mapFile;
#endif
    uint8_t* _buffer;
    std::string _id;
    std::string _usageId;
    int64_t _bufferSize;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class DoubleBufferedSharedMemorySidebandData : public SidebandData
{
public:
    DoubleBufferedSharedMemorySidebandData(const std::string& id, int64_t bufferSize);
    virtual ~DoubleBufferedSharedMemorySidebandData();

    void Write(uint8_t* bytes, int bytecount) override;
    void Read(uint8_t* bytes, int bufferSize, int* numBytesRead) override;

    const std::string& UsageId() override;

public:
    static DoubleBufferedSharedMemorySidebandData* InitNew(int64_t bufferSize);

private:
    std::string _id;
    SharedMemorySidebandData* _current;
    SharedMemorySidebandData _bufferA;
    SharedMemorySidebandData _bufferB;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class SocketSidebandData : public SidebandData
{
public:
    SocketSidebandData(uint64_t socket, const std::string& id);
    virtual ~SocketSidebandData();

    void Write(uint8_t* bytes, int bytecount) override;
    void Read(uint8_t* bytes, int bufferSize, int* numBytesRead) override;

    const std::string& UsageId() override;

public:
    static SocketSidebandData* ClientInit(const std::string& sidebandServiceUrl, const std::string& usageId);

private:
    std::string _id;
    uint64_t _socket;
};


//---------------------------------------------------------------------
//---------------------------------------------------------------------
class RdmaSidebandDataImp;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class RdmaSidebandData : public SidebandData
{
public:
    RdmaSidebandData(const std::string& id, RdmaSidebandDataImp* implementation);
    virtual ~RdmaSidebandData();

    void Write(uint8_t* bytes, int bytecount) override;
    void Read(uint8_t* bytes, int bufferSize, int* numBytesRead) override;
    const std::string& UsageId() override;

public:
    static RdmaSidebandData* ClientInit(const std::string& sidebandServiceUrl, const std::string& usageId, int64_t bufferSize);

private:
    std::unique_ptr<RdmaSidebandDataImp> _imp;
    std::string _id;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void AddServerSidebandSocket(int socket, const std::string& usageId);
void RegisterSidebandData(SidebandData* sidebandData);
std::vector<std::string> SplitUrlString(const std::string& s);

std::string GetConnectionAddress(::SidebandStrategy strategy);
std::string GetRdmaAddress();
std::string GetSocketsAddress();

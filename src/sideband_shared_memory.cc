//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include "client_utilities.h"
#include "FastMemcpy.h"
#include <fstream>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "sideband_data.h"
#include "sideband_internal.h"
#include "FastMemcpy.h"

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool _useFastMemcpy = false;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SetFastMemcpy(bool fastMemcpy)
{
    _useFastMemcpy = fastMemcpy;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SharedMemorySidebandData::SharedMemorySidebandData(const std::string& id, int64_t bufferSize) :
    _mapFile(INVALID_HANDLE_VALUE),
    _buffer(nullptr),
    _usageId(id),
    _id("TESTBUFFER_" + id),
    _bufferSize(bufferSize)
{
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SharedMemorySidebandData::~SharedMemorySidebandData()
{
    UnmapViewOfFile(_buffer);
    CloseHandle(_mapFile);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SharedMemorySidebandData* SharedMemorySidebandData::InitNew(int64_t bufferSize)
{    
    std::string usageId = "TestBuffer";
    auto sidebandData = new SharedMemorySidebandData(usageId, bufferSize);
    return sidebandData;
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
    if (_buffer == nullptr)
    {
        Init();
    }
    return _buffer;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SharedMemorySidebandData::Init()
{
    _mapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,    // use paging file
        NULL,                    // default security
        PAGE_READWRITE,          // read/write access
        0,                       // maximum object size (high-order DWORD)
        _bufferSize,             // maximum object size (low-order DWORD)
        _id.c_str());            // name of mapping object

    if (_mapFile == NULL)
    {
        std::cout << "Could not create file mapping object " << GetLastError() << std::endl;
        return;
    }
    _buffer = (uint8_t*)MapViewOfFile(_mapFile, FILE_MAP_ALL_ACCESS, 0, 0, _bufferSize);
    if (_buffer == NULL)
    {
        std::cout << "Could not map view of file " << GetLastError() << std::endl;
        CloseHandle(_mapFile);
    }
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
DoubleBufferedSharedMemorySidebandData::DoubleBufferedSharedMemorySidebandData(const std::string& id, int64_t bufferSize) :
    _bufferA(id + "_A", bufferSize),
    _bufferB(id + "_B", bufferSize),
    _id(id)
{
    _current = &_bufferA;    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
DoubleBufferedSharedMemorySidebandData::~DoubleBufferedSharedMemorySidebandData()
{    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
DoubleBufferedSharedMemorySidebandData* DoubleBufferedSharedMemorySidebandData::InitNew(int64_t bufferSize)
{    
    std::string usageId = "TestBuffer";
    auto sidebandData = new DoubleBufferedSharedMemorySidebandData(usageId, bufferSize);
    return sidebandData;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void DoubleBufferedSharedMemorySidebandData::Write(uint8_t* bytes, int bytecount)
{
    _current->Write(bytes, bytecount);
    _current = _current == &_bufferA ? &_bufferB : &_bufferA;    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void DoubleBufferedSharedMemorySidebandData::Read(uint8_t* bytes, int bufferSize, int* numBytesRead)
{    
    _current->Read(bytes, bufferSize, numBytesRead);
    _current = _current == &_bufferA ? &_bufferB : &_bufferA;    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
const std::string& DoubleBufferedSharedMemorySidebandData::UsageId()
{
    return _id;
}

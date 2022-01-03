//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include "client_utilities.h"
#include <fstream>
#include <sstream>
#include "sideband_data.h"
#include "sideband_internal.h"

#ifdef _WIN32
#include "FastMemcpy.h"
#endif

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
#ifdef _WIN32
    _mapFile(INVALID_HANDLE_VALUE),
#endif
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
#ifdef _WIN32
    UnmapViewOfFile(_buffer);
    CloseHandle(_mapFile);
#endif
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
#ifdef _WIN32
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
#endif
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SharedMemorySidebandData::Write(uint8_t* bytes, int bytecount)
{    
    auto ptr = GetBuffer();
#ifdef _WIN32
    if (_useFastMemcpy)
    {
        memcpy_fast(ptr, bytes, bytecount);
    }
    else
    {
        memcpy(ptr, bytes, bytecount);
    }
#else
        memcpy(ptr, bytes, bytecount);
#endif
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SharedMemorySidebandData::Read(uint8_t* bytes, int bufferSize, int* numBytesRead)
{    
    auto ptr = GetBuffer();
#ifdef _WIN32
    if (_useFastMemcpy)
    {
        memcpy_fast(bytes, ptr, bufferSize);
    }
    else
    {
        memcpy(bytes, ptr, bufferSize);
    }
#else
    memcpy(bytes, ptr, bufferSize);
#endif
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

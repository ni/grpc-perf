//---------------------------------------------------------------------
//---------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <client_utilities.h>

enum SidebandStrategy
{
  UNKNOWN = 0,
  SHARED_MEMORY = 1,
  DOUBLE_BUFFERED_SHARED_MEMORY = 2,
  SOCKETS = 3,
  HYPERVISOR_SOCKETS = 4,
  RDMA = 5,
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::string InitOwnerSidebandData(::SidebandStrategy strategy, int64_t num_samples);
int64_t GetOwnerSidebandDataToken(const std::string& usageId);
int64_t InitClientSidebandData(const std::string& sidebandServiceUrl, ::SidebandStrategy strategy, const std::string& usageId);
void WriteSidebandData(int64_t dataToken, uint8_t* bytes, int bytecount);
void ReadSidebandData(int64_t dataToken, uint8_t* bytes, int bufferSize, int* numBytesRead);
void CloseSidebandData(int64_t dataToken);
void AddServerSidebandSocket(int socket, const std::string& usageId);
int RunSidebandSocketsAccept();

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SetFastMemcpy(bool fastMemcpy);

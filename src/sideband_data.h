//---------------------------------------------------------------------
//---------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <client_utilities.h>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
enum class SidebandStrategy
{
  UNKNOWN = 0,
  GRPC = 1,
  SHARED_MEMORY = 2,
  DOUBLE_BUFFERED_SHARED_MEMORY = 3,
  SOCKETS = 4,
  SOCKETS_LOW_LATENCY = 5,
  HYPERVISOR_SOCKETS = 6,
  RDMA = 7,
  RDMA_LOW_LATENCY = 8
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::string InitOwnerSidebandData(::SidebandStrategy strategy, int64_t bufferSize);
int64_t GetOwnerSidebandDataToken(const std::string& usageId);
int64_t InitClientSidebandData(const std::string& sidebandServiceUrl, ::SidebandStrategy strategy, const std::string& usageId, int bufferSize);
void WriteSidebandData(int64_t dataToken, uint8_t* bytes, int64_t bytecount);
void ReadSidebandData(int64_t dataToken, uint8_t* bytes, int64_t bufferSize, int64_t* numBytesRead);
void CloseSidebandData(int64_t dataToken);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int64_t InitClientSidebandData(const BeginMonikerSidebandStreamResponse& response);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int RunSidebandSocketsAccept();
int AcceptSidebandRdmaSendRequests();
int AcceptSidebandRdmaReceiveRequests();

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void QueueSidebandConnection(::SidebandStrategy strategy, bool waitForReader, bool waitForWriter, int64_t bufferSize);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool ReadSidebandMessage(int64_t dataToken, google::protobuf::MessageLite* writeRequest);
int64_t WriteSidebandMessage(int64_t dataToken, const google::protobuf::MessageLite& response);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int64_t InitMonikerSidebandData(const BeginMonikerSidebandStreamResponse& initResponse);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SetFastMemcpy(bool fastMemcpy);

#include <map>
//#include "absl/log/check.h"
#include <grpcpp/support/client_interceptor.h>
#include <sharedmem_client_interceptor.h>
#include <sideband_data.h>
#include <perftest.pb.h>
#include <google/protobuf/io/tokenizer.h>


static HANDLE StartCallEvent = INVALID_HANDLE_VALUE;
static HANDLE CallCompleteEvent = INVALID_HANDLE_VALUE;
int64_t sideband_token = 0;
uint8_t* sideband_memory = nullptr;
unsigned int* _clientReadReady = nullptr;
unsigned int* _clientWriteReady = nullptr;

class SharedMemoryForwardingInterceptor : public grpc::experimental::Interceptor 
{    
public:
    SharedMemoryForwardingInterceptor(grpc::experimental::ClientRpcInfo* info) 
    {
        info_ = info;
        if (sideband_token == 0)
        {
            StartCallEvent = CreateEvent(nullptr, false, false, "StartCallEvent");
            CallCompleteEvent = CreateEvent(nullptr, false, false, "CallCompleteEvent");
            InitClientSidebandData("TestBuffer", SidebandStrategy::SHARED_MEMORY, "TestBuffer", 4096, &sideband_token);
            SidebandData_BeginDirectWrite(sideband_token, &sideband_memory);
            unsigned int* locks = (unsigned int*)sideband_memory;
            _clientWriteReady = locks;
            _clientReadReady = locks + 1;
            sideband_memory += 8;
        }
    }

    void SignalReady()
    {
        //SetEvent(StartCallEvent);
        InterlockedExchange(_clientWriteReady, 1);
    }

    void WaitForReadReady()
    {
        //WaitForSingleObject(CallCompleteEvent, INFINITE);
        while (InterlockedCompareExchange(_clientReadReady, 0, 1) == 0);
    }

    void Intercept(::grpc::experimental::InterceptorBatchMethods* methods) override 
    {
        bool hijack = false;
        if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_INITIAL_METADATA)) {
            // Hijack all calls
            hijack = true;
            // Create a stream on which this interceptor can make requests
            // stub_ = keyvaluestore::KeyValueStore::NewStub(methods->GetInterceptedChannel());
            // stream_ = stub_->GetValues(&context_);
            //std::cout << "Pre Send Initial Metadata" << std::endl;
        }
        if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_MESSAGE)) {
            // We know that clients perform a Read and a Write in a loop, so we don't
            // need to maintain a list of the responses.
            // std::string requested_key;

            auto packedRequest = sideband_memory;
            auto method = info_->method();
            auto methodLen = strlen(method);
            *(int32_t*)packedRequest = methodLen;
            packedRequest += 4;
            memcpy(packedRequest, method, methodLen);
            packedRequest += methodLen;
            grpc::Slice requestMessage;
            methods->GetSerializedSendMessage()->DumpToSingleSlice(&requestMessage);
            *(int32_t*)packedRequest = requestMessage.size();
            packedRequest += 4;
            memcpy(packedRequest, requestMessage.begin(), requestMessage.size());
            //SetEvent(StartCallEvent);
            SignalReady();
            auto details = methods->GetSendInitialMetadata();
            //std::cout << "Pre Send Message" << std::endl;
        }
        if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_CLOSE)) 
        {
            // stream_->WritesDone();
            //std::cout << "Pre Send Close" << std::endl;
        }
        if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_RECV_MESSAGE)) 
        {
            //keyvaluestore::Response* resp = static_cast<keyvaluestore::Response*>(methods->GetRecvMessage());
            //resp->set_value(response_);
            google::protobuf::MessageLite* result = static_cast<google::protobuf::MessageLite*>(methods->GetRecvMessage());
            //WaitForSingleObject(CallCompleteEvent, INFINITE);
            WaitForReadReady();
            //niPerfTest::InitResult r;
            //r.set_status(42);
            //void* data = new int8_t[4096];
            //r.SerializeToArray(data, 4096);
            result->ParseFromArray(sideband_memory, 4096);
            //std::cout << "Pre Receive Message" << std::endl;
        }
        if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_RECV_STATUS)) 
        {
            auto* status = methods->GetRecvStatus();
            *status = grpc::Status::OK;
            //std::cout << "Pre Receive Status" << std::endl;
        }
        // One of Hijack or Proceed always needs to be called to make progress.
        if (hijack) {
            // Hijack is called only once when PRE_SEND_INITIAL_METADATA is present in
            // the hook points
            methods->Hijack();
        } 
        else 
        {
            // Proceed is an indicator that the interceptor is done intercepting the
            // batch.
            methods->Proceed();
        }
  }

 private:
     grpc::experimental::ClientRpcInfo* info_;
    //grpc::ClientContext context_;
    //std::unique_ptr<keyvaluestore::KeyValueStore::Stub> stub_;
    //std::unique_ptr<grpc::ClientReaderWriter<keyvaluestore::Request, keyvaluestore::Response>> stream_;
    //std::map<std::string, std::string> cached_map_;
    //std::string response_;
};

grpc::experimental::Interceptor* SharedMemoryForwardingInterceptorFactory::CreateClientInterceptor(grpc::experimental::ClientRpcInfo* info)
{
    return new SharedMemoryForwardingInterceptor(info);
}

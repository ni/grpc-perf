#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <atomic>
#include <sched.h>
#include <fcntl.h>
#include <map>
#include <grpcpp/support/client_interceptor.h>
#include <sharedmem_client_interceptor.h>
#include <sideband_data.h>
#include <perftest.pb.h>
#include <google/protobuf/io/tokenizer.h>
#include "iox2/iceoryx2.hpp"

// Define a simple payload structure for iceoryx2 communication
struct MinimalPayload {
   uint8_t buffer[512];
   int32_t data;
};

int64_t sideband_token = 0;

class SharedMemoryForwardingInterceptor : public grpc::experimental::Interceptor 
{    
private:
    // Static members to be shared across all instances - use void* for simplicity
    std::optional<iox2::Node<iox2::ServiceType::Ipc>> node_;
    static inline std::optional<iox2::PortFactoryRequestResponse<iox2::ServiceType::Ipc, MinimalPayload, void, MinimalPayload, void>> service_;
    static inline std::optional<iox2::Client<iox2::ServiceType::Ipc, MinimalPayload, void, MinimalPayload, void>> client_;

    std::optional<iox2::PendingResponse<iox2::ServiceType::Ipc, MinimalPayload, void, MinimalPayload, void>> pendingResponse_;
    
public:

    SharedMemoryForwardingInterceptor(grpc::experimental::ClientRpcInfo* info) 
    {
        info_ = info;
        if (sideband_token == 0)
        {
            std::cout << "Setting Things Up!" << std::endl;
            // Initialize static members only once
            node_.emplace(iox2::NodeBuilder().create<iox2::ServiceType::Ipc>().expect("successful node creation"));
            std::cout << "Setting Things Up -- NODE CREATED!" << std::endl;

            service_.emplace(node_->service_builder(iox2::ServiceName::create("GRPCBenchmarkService").expect("valid service name"))
                            .request_response<MinimalPayload, MinimalPayload>()
                            //.max_response_buffer_size(1)  // Minimize buffer size
                            .open_or_create().expect("successful service creation"));
            std::cout << "Setting Things Up -- NODE CREATED!" << std::endl;
            client_.emplace(service_->client_builder().create().expect("successful client creation"));
           
            std::cout << "Setting Things Up -- DONE!" << std::endl;
            sideband_token = 1;
        }
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

            auto request = client_->loan_uninit().expect("loan successful");
            MinimalPayload payload;
            auto packedRequest = payload.buffer;

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
            auto written = request.write_payload(std::move(payload));
            pendingResponse_.emplace(iox2::send(std::move(written)).expect("send successful"));
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

            while (true) {
                auto response = pendingResponse_->receive().expect("receive successful");
                if (response.has_value()) {
                    result->ParseFromArray(response->payload().buffer, 4096);
                    break;
                }
            }
            // niPerfTest::InitResult r;
            // r.set_status(42);
            // void* data = new int8_t[4096];
            // r.SerializeToArray(data, 4096);
            // result->ParseFromArray(data, 4096);
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
    // Instance member
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

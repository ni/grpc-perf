//---------------------------------------------------------------------
// Defimnition of the NIPerfTestServer and MonikerServer gRPC Server
//---------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#ifdef __WIN32__
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <perftest.grpc.pb.h>
#include <condition_variable>
#include <thread>
#include <sstream>
#include <fstream>
#include <iostream>
#include <perftest.grpc.pb.h>
#include <thread>
#include <memory>

#ifndef _WIN32
#include <sched.h>
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class NIPerfTestProxyServer final : public niPerfTest::niPerfTestService::Service
{
public:
    grpc::Status StreamLatencyTest(grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::niPerfTest::StreamLatencyServer, niPerfTest::StreamLatencyClient>* stream) override;
    grpc::Status StreamLatencyTestClient(grpc::ServerContext* context, ::grpc::ServerReader< ::niPerfTest::StreamLatencyClient>* reader, niPerfTest::StreamLatencyServer* response) override;
    grpc::Status StreamLatencyTestServer(grpc::ServerContext* context, const ::niPerfTest::StreamLatencyClient* request, ::grpc::ServerWriter<niPerfTest::StreamLatencyServer>* writer) override;
    Status Init(ServerContext* context, const niPerfTest::InitParameters*, niPerfTest::InitResult* response) override;
    Status ConfigureVertical(grpc::ServerContext* context, const niPerfTest::ConfigureVerticalRequest* request, niPerfTest::ConfigureVerticalResponse* response) override;
    Status ConfigureHorizontalTiming(grpc::ServerContext* context, const niPerfTest::ConfigureHorizontalTimingRequest* request, niPerfTest::ConfigureHorizontalTimingResponse* response) override;
    Status InitiateAcquisition(grpc::ServerContext* context, const niPerfTest::InitiateAcquisitionRequest* request, niPerfTest::InitiateAcquisitionResponse* response) override;
    Status Read(ServerContext* context, const niPerfTest::ReadParameters* request, niPerfTest::ReadResult* response) override;
    Status ReadContinuously(ServerContext* context, const niPerfTest::ReadContinuouslyParameters* request, grpc::ServerWriter<niPerfTest::ReadContinuouslyResult>* writer) override;
    Status TestWrite(ServerContext* context, const niPerfTest::TestWriteParameters* request, niPerfTest::TestWriteResult* response) override;
    Status TestWriteContinuously(ServerContext* context, grpc::ServerReaderWriter<niPerfTest::TestWriteResult, niPerfTest::TestWriteParameters>* stream) override;

    void InitStub();

private:
    std::unique_ptr<niPerfTest::niPerfTestService::Stub> _stub;
};


//---------------------------------------------------------------------
//---------------------------------------------------------------------
void NIPerfTestProxyServer::InitStub()
{
    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_MINIMAL_STACK, 1);
    args.SetMaxReceiveMessageSize(10 * 100 * 1024 * 1024);
    args.SetMaxSendMessageSize(10 * 100 * 1024 * 1024);
    _stub = niPerfTest::niPerfTestService::NewStub(grpc::CreateCustomChannel("localhost:50051", grpc::InsecureChannelCredentials(), args));
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
grpc::Status NIPerfTestProxyServer::StreamLatencyTestClient(grpc::ServerContext* context, grpc::ServerReader<niPerfTest::StreamLatencyClient>* reader, niPerfTest::StreamLatencyServer* response)
{
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
grpc::Status NIPerfTestProxyServer::StreamLatencyTestServer(grpc::ServerContext* context, const niPerfTest::StreamLatencyClient* request, grpc::ServerWriter<niPerfTest::StreamLatencyServer>* writer)
{
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestProxyServer::StreamLatencyTest(ServerContext* context, grpc::ServerReaderWriter<niPerfTest::StreamLatencyServer, niPerfTest::StreamLatencyClient>* stream)
{
    grpc::ClientContext clientContext;
    niPerfTest::StreamLatencyClient readParameters;
    niPerfTest::StreamLatencyServer response;    

    auto writer = _stub->StreamLatencyTest(&clientContext);
    while (stream->Read(&readParameters))
    {
        writer->Write(readParameters);
        writer->Read(&response);
        stream->Write(response);
    }
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestProxyServer::Init(ServerContext* context, const niPerfTest::InitParameters* request, niPerfTest::InitResult* response)
{
    grpc::ClientContext clientContext;
    return _stub->Init(&clientContext, *request, response);
}


//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestProxyServer::ConfigureVertical(grpc::ServerContext* context, const niPerfTest::ConfigureVerticalRequest* request, niPerfTest::ConfigureVerticalResponse* response)
{    
    grpc::ClientContext clientContext;
    return _stub->ConfigureVertical(&clientContext, *request, response);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestProxyServer::ConfigureHorizontalTiming(grpc::ServerContext* context, const niPerfTest::ConfigureHorizontalTimingRequest* request, niPerfTest::ConfigureHorizontalTimingResponse* response)
{    
    grpc::ClientContext clientContext;
    return _stub->ConfigureHorizontalTiming(&clientContext, *request, response);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestProxyServer::InitiateAcquisition(grpc::ServerContext* context, const niPerfTest::InitiateAcquisitionRequest* request, niPerfTest::InitiateAcquisitionResponse* response)
{    
    grpc::ClientContext clientContext;
    return _stub->InitiateAcquisition(&clientContext, *request, response);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestProxyServer::Read(ServerContext* context, const niPerfTest::ReadParameters* request, niPerfTest::ReadResult* response)
{	
    grpc::ClientContext clientContext;
    return _stub->Read(&clientContext, *request, response);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestProxyServer::TestWrite(ServerContext* context, const niPerfTest::TestWriteParameters* request, niPerfTest::TestWriteResult* response)
{
    grpc::ClientContext clientContext;
    return _stub->TestWrite(&clientContext, *request, response);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestProxyServer::TestWriteContinuously(ServerContext* context, grpc::ServerReaderWriter<niPerfTest::TestWriteResult, niPerfTest::TestWriteParameters>* stream)
{
    grpc::ClientContext clientContext;
    niPerfTest::TestWriteParameters readParameters;
    niPerfTest::TestWriteResult response;    

    auto writer = _stub->TestWriteContinuously(&clientContext);
    while (stream->Read(&readParameters))
    {
        writer->Write(readParameters);
        writer->Read(&response);
        stream->Write(response);
    }
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestProxyServer::ReadContinuously(ServerContext* context, const niPerfTest::ReadContinuouslyParameters* request, grpc::ServerWriter<niPerfTest::ReadContinuouslyResult>* writer)
{
    grpc::ClientContext clientContext;
    niPerfTest::ReadContinuouslyResult response;

    auto reader = _stub->ReadContinuously(&clientContext, *request);
    while (reader->Read(&response))
    {
        writer->Write(response);
    }
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class AsyncCall
{
public:
    virtual void Proceed(bool ok) = 0;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class ReadAsyncData : AsyncCall
{
public:
    void Proceed(bool ok) override;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class WriteAsyncData : AsyncCall
{    
public:
    void Proceed(bool ok) override;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class StartAsyncCall : public AsyncCall
{
public:
    StartAsyncCall(grpc::ServerCompletionQueue* cq, grpc::AsyncGenericService* service);
    void Proceed(bool ok) override;

public:
    grpc::ServerCompletionQueue* _cq;
    grpc::AsyncGenericService* _service;
    // grpc::GenericServerContext _ctx;
    // grpc::GenericServerAsyncReaderWriter _stream;
};

StartAsyncCall::StartAsyncCall(grpc::ServerCompletionQueue* cq, grpc::AsyncGenericService* service) :
    _cq(cq),
    _service(service)
{
}

void StartAsyncCall::Proceed(bool ok)
{    
    //auto name = _ctx.method();

    // auto clientCall = new grpc_labview::BidiStreamingClientCall();
    // *callId = clientCall;
    // clientCall->request = std::make_shared<grpc_labview::LVMessage>(requestMetadata);
    // clientCall->response = std::make_shared<grpc_labview::LVMessage>(responseMetadata);

    // grpc::internal::RpcMethod method(name.c_str(), grpc::internal::RpcMethod::BIDI_STREAMING);
    // grpc::ClientContext context;
    // std::shared_ptr<grpc::Channel> channel;

    // auto readerWriter = grpc_impl::internal::ClientReaderWriterFactory<google::protobuf::Message, google::protobuf::Message>::Create(channel.get(), method, &context);
    //clientCall->_readerWriter = std::shared_ptr<grpc_impl::ClientReaderWriterInterface<grpc_labview::LVMessage, grpc_labview::LVMessage>>(readerWriter);

    // readerWriter->Read()
    // readerWriter->Write();

    // return 0;    

}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void HandleAsyncRequests(grpc::ServerCompletionQueue* cq, grpc::AsyncGenericService* service)
{
    // auto nextCall = new StartAsyncCall(cq, service);
    // service->RequestCall(&nextCall->_ctx, &nextCall->_stream, cq, cq, nextCall);

    // void *tag;
    // bool ok;
    // while (true)
    // {
    //     cq->Next(&tag, &ok);
    //     static_cast<AsyncCall*>(tag)->Proceed(ok);
    //     if (!ok)
    //     {
    //         break;
    //     }
    // }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunServer(int argc, char **argv, const char* server_address)
{
	auto creds = grpc::InsecureServerCredentials();

	NIPerfTestProxyServer service;
    service.InitStub();

	ServerBuilder builder;
	builder.AddListeningPort(server_address, creds);
    builder.AddChannelArgument(GRPC_ARG_MINIMAL_STACK, 1);
	builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE);
	builder.SetMaxMessageSize(1 * 1024 * 1024);
	builder.SetMaxReceiveMessageSize(1 * 1024 * 1024);
	builder.RegisterService(&service);

    auto genericService = std::unique_ptr<grpc::AsyncGenericService>(new grpc::AsyncGenericService());
    builder.RegisterAsyncGenericService(genericService.get());
    auto cq = builder.AddCompletionQueue();

	auto server = builder.BuildAndStart();
	std::cout << "Proxy Server listening on " << server_address << std::endl;

    HandleAsyncRequests(cq.get(), genericService.get());
	server->Wait();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int main(int argc, char **argv)
{
    //grpc_init();
    //grpc_timer_manager_set_threading(false);
    //grpc_core::Executor::SetThreadingDefault(false);
    //grpc_core::Executor::SetThreadingAll(false);
#ifndef _WIN32    
    sched_param schedParam;
    schedParam.sched_priority = 95;
    sched_setscheduler(0, SCHED_FIFO, &schedParam);
    // cpu_set_t cpuSet;
    // CPU_ZERO(&cpuSet);
    // CPU_SET(2, &cpuSet);
    // sched_setaffinity(1, sizeof(cpu_set_t), &cpuSet);
#else
    DWORD dwError, dwPriClass;
    if(!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
    {
        dwError = GetLastError();
        if( ERROR_PROCESS_MODE_ALREADY_BACKGROUND == dwError)
            std::cout << "Already in background mode" << std::endl;
        else
            std::cout << "Failed change priority: " << dwError << std::endl;
   } 
#endif

    std::vector<std::thread*> threads;
    std::vector<std::string> ports;
    for (int x=0; x<1; ++x)
    {
        auto port = 50053 + x;
        auto portStr = std::string("0.0.0.0:") + std::to_string(port);
        ports.push_back(portStr);
    } 
    for (auto port: ports)
    {
        auto p = new std::string(port.c_str());
        auto t = new std::thread(RunServer, 0, argv, p->c_str());
        threads.push_back(t);
    }
    for (auto t: threads)
    {
        t->join();
    }    


	return 0;
}

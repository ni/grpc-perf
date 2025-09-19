//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <atomic>
#include <sched.h>
#include <fcntl.h>

#include <client_utilities.h>

#if ENABLE_GRPC_SIDEBAND
#include <sideband_data.h>
#include <sideband_grpc.h>
#endif
// #include <boost/interprocess/sync/named_semaphore.hpp>
// #include <sys/mman.h>

#include <performance_tests.h>
#include <grpcpp/impl/codegen/client_context.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/support/channel_arguments.h>
#include <thread>
#include <sstream>
#include <fstream>
#include <iostream>
#include <perftest_server.h>
#include <perftest.grpc.pb.h>
#include <thread>
#include <sideband_data.h>

#ifndef _WIN32
// #include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
//#include <sched.h>
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using namespace niPerfTest;
using namespace google::protobuf;
using namespace ni::data_monikers;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using timeVector = std::vector<std::chrono::microseconds>;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
static ::grpc::ServerWriter<niPerfTest::StreamLatencyServer>* _writers [32];

//---------------------------------------------------------------------
//---------------------------------------------------------------------
static bool useAnyType;
static std::shared_ptr<grpc::Channel> _inProcServer;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
grpc::Status NIPerfTestServer::StreamLatencyTestClient(grpc::ServerContext* context, grpc::ServerReader<niPerfTest::StreamLatencyClient>* reader, niPerfTest::StreamLatencyServer* response)
{	
	niPerfTest::StreamLatencyClient client;
	niPerfTest::StreamLatencyServer server;
    uint32_t slot;
    bool first = true;
	while (reader->Read(&client))
	{
        slot = client.message();
        if (first)
        {
            first = false;
#ifndef _WIN32    
            cpu_set_t cpuSet;
            CPU_ZERO(&cpuSet);
            CPU_SET(slot, &cpuSet);
            sched_setaffinity(0, sizeof(cpu_set_t), &cpuSet);
#endif
        }
        if (_writers[slot] != nullptr)
        {
            _writers[slot]->Write(server);
        }
 	}
     _writers[slot] = nullptr;
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
grpc::Status NIPerfTestServer::StreamLatencyTestServer(grpc::ServerContext* context, const niPerfTest::StreamLatencyClient* request, grpc::ServerWriter<niPerfTest::StreamLatencyServer>* writer)
{
    auto slot = request->message();
    _writers[slot] = writer;
    while (_writers[slot] == writer)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::StreamLatencyTest(ServerContext* context, grpc::ServerReaderWriter<niPerfTest::StreamLatencyServer, niPerfTest::StreamLatencyClient>* stream)
{
	niPerfTest::StreamLatencyClient client;
	niPerfTest::StreamLatencyServer server;
	while (stream->Read(&client))
	{
        //server.set_message(client.message());
		stream->Write(server);
	}
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::Init(ServerContext* context, const niPerfTest::InitParameters* request, niPerfTest::InitResult* response)
{
	response->set_status(request->id());
	return Status::OK;	
}


//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::ConfigureVertical(grpc::ServerContext* context, const niPerfTest::ConfigureVerticalRequest* request, niPerfTest::ConfigureVerticalResponse* response)
{    
	return Status::OK;	
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::ConfigureHorizontalTiming(grpc::ServerContext* context, const niPerfTest::ConfigureHorizontalTimingRequest* request, niPerfTest::ConfigureHorizontalTimingResponse* response)
{    
	return Status::OK;	
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::InitiateAcquisition(grpc::ServerContext* context, const niPerfTest::InitiateAcquisitionRequest* request, niPerfTest::InitiateAcquisitionResponse* response)
{    
	return Status::OK;	
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::Read(ServerContext* context, const niPerfTest::ReadParameters* request, niPerfTest::ReadResult* response)
{	
	response->mutable_samples()->Reserve(request->num_samples());
	response->mutable_samples()->Resize(request->num_samples(), 8.325793493);
	response->set_status(0);
	return Status::OK;
}

//---------------------------------------------------------------------
Status NIPerfTestServer::ReadComplex(ServerContext* context, const niPerfTest::ReadParameters* request, niPerfTest::ReadComplexResult* response)
{	
	response->mutable_samples()->Reserve(request->num_samples());
    for (int x=0; x<request->num_samples(); ++x)
    {
        auto sample = new niPerfTest::ComplexNumber();
        sample->set_real(3.14);
        sample->set_imaginary(4.56);
	    response->mutable_samples()->AddAllocated(sample);
    }
	response->set_status(0);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::TestWrite(ServerContext* context, const niPerfTest::TestWriteParameters* request, niPerfTest::TestWriteResult* response)
{
	response->set_status(0);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::TestWriteContinuously(ServerContext* context, grpc::ServerReaderWriter<niPerfTest::TestWriteResult, niPerfTest::TestWriteParameters>* stream)
{
    niPerfTest::TestWriteParameters readParameters;
    niPerfTest::TestWriteResult response;    
    response.set_status(0);
    while (stream->Read(&readParameters))
    {
        stream->Write(response);
    }
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::ReadContinuously(ServerContext* context, const niPerfTest::ReadContinuouslyParameters* request, grpc::ServerWriter<niPerfTest::ReadContinuouslyResult>* writer)
{			
	niPerfTest::ReadContinuouslyResult response;
	response.mutable_wfm()->Reserve(request->num_samples());
	response.mutable_wfm()->Resize(request->num_samples(), 0.0);
    auto iterations = request->num_iterations();
    if (iterations == 0)
    {
        iterations = 10000;
    }
	for (int x=0; x<iterations; ++x)
	{
		writer->Write(response);
	}
	return Status::OK;
}

#if ENABLE_GRPC_SIDEBAND
//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::BeginTestSidebandStream(ServerContext* context, const niPerfTest::BeginTestSidebandStreamRequest* request, niPerfTest::BeginTestSidebandStreamResponse* response)
{    
    char identifier[32] = {};
    InitOwnerSidebandData((::SidebandStrategy)request->strategy(), request->num_samples(), identifier);
    response->set_strategy(request->strategy());
    response->set_sideband_identifier(identifier);
    char address[1024] = {};
    GetSidebandConnectionAddress((::SidebandStrategy)request->strategy(), address);
    response->set_connection_url(address);

    QueueSidebandConnection((::SidebandStrategy)request->strategy(), identifier, true, true, request->num_samples());
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::TestSidebandStream(ServerContext* context, grpc::ServerReaderWriter<niPerfTest::TestSidebandStreamResponse, niPerfTest::TestSidebandStreamRequest>* stream)
{
    uint8_t* buffer = new uint8_t[16 * 1024 * 1024];
    TestSidebandStreamRequest request;
    bool firstWrite = true;
    int64_t sidebandToken = 0;
    while (stream->Read(&request))
    {
        TestSidebandStreamResponse response;
        if (sidebandToken == 0)
        {
            GetOwnerSidebandDataToken(request.sideband_identifier().c_str(), &sidebandToken);
            assert(sidebandToken != 0);
        }
        switch (request.strategy())
        {
            case ni::data_monikers::SidebandStrategy::RDMA:
            case ni::data_monikers::SidebandStrategy::RDMA_LOW_LATENCY:
            case ni::data_monikers::SidebandStrategy::SOCKETS:
            case ni::data_monikers::SidebandStrategy::SOCKETS_LOW_LATENCY:
                stream->Write(response);
                WriteSidebandData(sidebandToken, buffer, request.num_samples());
                break;
            case ni::data_monikers::SidebandStrategy::SHARED_MEMORY:
                WriteSidebandData(sidebandToken, buffer, request.num_samples());
                stream->Write(response);
                break;
            case ni::data_monikers::SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY:
                if (firstWrite)
                {
                    firstWrite = false;
                    WriteSidebandData(sidebandToken, buffer, request.num_samples());
                }
                stream->Write(response);
                WriteSidebandData(sidebandToken, buffer, request.num_samples());
                break;
        }
    }
    CloseSidebandData(sidebandToken);
    return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunSidebandReadWriteLoop(const char* sidebandIdentifier, ::SidebandStrategy strategy)
{
    TestSidebandStreamResponse response;
    int64_t sidebandToken = 0;
    GetOwnerSidebandDataToken(sidebandIdentifier, &sidebandToken);
    assert(sidebandToken != 0);

    std::cout << "Starting sideband loop" << std::endl;
    while (true)
    {
        SidebandWriteRequest request;
        SidebandReadResponse response;
        if (!ReadSidebandMessage(sidebandToken, &request))
        {
            break;
        }
        if (request.cancel())
        {
            break;
        }
        auto result = response.mutable_values()->add_values();
        result->CopyFrom(request.values().values().at(0));
        if (!WriteSidebandMessage(sidebandToken, response))
        {
            break;
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    delete [] sidebandIdentifier;
    CloseSidebandData(sidebandToken);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
grpc::Status NIMonikerServer::BeginSidebandStream(::grpc::ServerContext* context, const ::ni::data_monikers::BeginMonikerSidebandStreamRequest* request, ::ni::data_monikers::BeginMonikerSidebandStreamResponse* response)
{
    auto bufferSize = 1024 * 1024;
    auto strategy = static_cast<::SidebandStrategy>(request->strategy());

    char* identifier = new char[32];
    InitOwnerSidebandData(strategy, bufferSize, identifier);
    char address[1024] = {};
    GetSidebandConnectionAddress((::SidebandStrategy)request->strategy(), address);
    response->set_strategy(request->strategy());
    response->set_sideband_identifier(identifier);
    response->set_connection_url(address);
    response->set_buffer_size(bufferSize);
    QueueSidebandConnection(strategy, identifier, true, true, bufferSize);

    auto thread = new std::thread(RunSidebandReadWriteLoop, identifier, strategy);
    thread->detach();

    return Status::OK;
}
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
grpc::Status NIMonikerServer::StreamReadWrite(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::ni::data_monikers::MonikerReadResult, ::ni::data_monikers::MonikerWriteRequest>* stream)
{    
    return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
grpc::Status NIMonikerServer::StreamRead(::grpc::ServerContext* context, const ::ni::data_monikers::MonikerList* request, ::grpc::ServerWriter< ::ni::data_monikers::MonikerReadResult>* writer)
{    
    return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
grpc::Status NIMonikerServer::StreamWrite(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::ni::data_monikers::StreamWriteResponse, ::ni::data_monikers::MonikerWriteRequest>* stream)
{    
    return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::string read_keycert( const std::string& filename)
{	
	std::string data;
	std::ifstream file(filename.c_str(), std::ios::in);
	if (file.is_open())
	{
		std::stringstream ss;
		ss << file.rdbuf();
		file.close();
		data = ss.str();
	}
	return data;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::shared_ptr<grpc::ServerCredentials> CreateCredentials(const std::string& certPath)
{
	std::shared_ptr<grpc::ServerCredentials> creds;
	if (!certPath.empty())
	{
		std::string servercert = read_keycert(certPath + ".crt");
		std::string serverkey = read_keycert(certPath + ".key");

		grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp;
		pkcp.private_key = serverkey;
		pkcp.cert_chain = servercert;

		grpc::SslServerCredentialsOptions ssl_opts;
		ssl_opts.pem_root_certs="";
		ssl_opts.pem_key_cert_pairs.push_back(pkcp);

		creds = grpc::SslServerCredentials(ssl_opts);
	}
	else
	{
		creds = grpc::InsecureServerCredentials();
	}
	return creds;
}

ReadComplexAsyncCall::ReadComplexAsyncCall() :
    _response(&_context)
{
    _complete = false;    
}
    
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void ReadComplexAsyncCall::HandleCall(bool ok)
{ 
    if (!_complete)
    {
        Arena arena;
        auto response = Arena::CreateMessage<niPerfTest::ReadComplexResult>(&arena);
        auto numSamples = _request.num_samples();
        response->mutable_samples()->Reserve(numSamples);
        for (int x=0; x<numSamples; ++x)
        {
            auto sample = response->mutable_samples()->Add();
            sample->set_real(3.14);
            sample->set_imaginary(4.56);
        }
        response->set_status(0);

        _response.Finish(*response, Status::OK, this);
        _complete = true;
    }
    else
    {
        delete this;
    }
}

#define SHM_NAME "/my_spin_semaphore"

typedef struct {
    std::atomic<int> count;
} spin_semaphore_t;

spin_semaphore_t *create_or_open_semaphore(const char *name, int initial_count, int create) {
    int flags = O_RDWR;
    if (create) flags |= O_CREAT;

    int fd = shm_open(name, flags, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }

    if (create) {
        if (ftruncate(fd, sizeof(spin_semaphore_t)) == -1) {
            perror("ftruncate");
            exit(1);
        }
    }

    spin_semaphore_t* sem = (spin_semaphore_t*)mmap(NULL, sizeof(spin_semaphore_t),
                                 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sem == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    if (create) {
        atomic_init(&sem->count, initial_count);
    }
    close(fd); // fd no longer needed after mmap
    return sem;
}

void spin_wait(spin_semaphore_t *sem) {
    int val;
    do {
        while (std::atomic_load_explicit(&sem->count, std::memory_order_relaxed) <= 0)
            sched_yield();
        val = std::atomic_fetch_sub_explicit(&sem->count, 1, std::memory_order_acquire);
    } while (val <= 0);
}

void spin_post(spin_semaphore_t *sem) {
    std::atomic_fetch_add_explicit(&sem->count, 1, std::memory_order_release);
}

// struct SharedAtomic {
//     std::atomic<int> flag;
// };

// boost::interprocess::named_semaphore _startCallEvent(boost::interprocess::open_or_create, "StartCallEvent", 0);
// boost::interprocess::named_semaphore _callCompleteEvent(boost::interprocess::open_or_create, "CallCompleteEvent", 0);

class SharedMemoryListener
{
    volatile int32_t* _readReady = nullptr;
    volatile int32_t* _writeReady = nullptr;
    // SharedAtomic* _readReady = nullptr;
    // SharedAtomic* _writeReady = nullptr;
    //HANDLE _startCallEvent;
    //HANDLE _callCompleteEvent;
    spin_semaphore_t* _startCallEvent = nullptr;
    spin_semaphore_t* _callCompleteEvent = nullptr;

    void SignalReady()
    {
        //SetEvent(_callCompleteEvent);
        //InterlockedExchange(_readReady, 1);
        //_readReady->flag.store(1, std::memory_order_release);
        //*_readReady = 1;
        //_callCompleteEvent.post();
        spin_post(_callCompleteEvent);
    }

    void WaitForReadReady()
    {
        //std::cout << "Waiting for call start" << std::endl;
        //WaitForSingleObject(startCallEvent, INFINITE);
        // int expected = 1;
        // do { expected = 1; _writeReady->flag.compare_exchange_strong(expected, 0); } while (expected == 0);
        // while (*_writeReady == 0)
        // {
        //     // Spin until the write is ready
        //     //std::this_thread::yield();
        // }
        // *_writeReady = 0;
        //std::cout << "Call started" << std::endl;
        //while (InterlockedCompareExchange(_writeReady, 0, 1) == 0);
        //_startCallEvent.wait();
        spin_wait(_startCallEvent);
    }

public:
    SharedMemoryListener()
    {    
        _startCallEvent = create_or_open_semaphore("/StartCallEvent", 0, true);
        _callCompleteEvent = create_or_open_semaphore("/CallCompleteEvent", 0, true);
    }

    void Run()
    {
        int64_t sideband_token = 0;
        uint8_t* sideband_memory = nullptr;
        char sidebandId[32];
        InitOwnerSidebandData(::SidebandStrategy::SHARED_MEMORY, 4096, sidebandId);
        GetOwnerSidebandDataToken(sidebandId, &sideband_token);
        SidebandData_BeginDirectWrite(sideband_token, &sideband_memory);
        // unsigned int* locks = (unsigned int*)sideband_memory;
        // //_writeReady = new(locks) SharedAtomic();
        // _writeReady = reinterpret_cast<volatile int32_t*>(locks);
        // //_readReady = new (locks + sizeof(SharedAtomic)) SharedAtomic();
        // _readReady = reinterpret_cast<volatile int32_t*>(locks) + 1;
        // sideband_memory += sizeof(int32_t) * 2;

        while (true)
        {
            WaitForReadReady();
            auto packedRequest = sideband_memory;
            auto methodLen = *(int32_t*)packedRequest;
            packedRequest += 4;
            std::string methodName((char*)packedRequest, methodLen);
            packedRequest += methodLen;
            auto requestLen = *(int32_t*)packedRequest;
            packedRequest += 4;

            grpc::internal::RpcMethod method(methodName.c_str(), grpc::internal::RpcMethod::NORMAL_RPC);
            ClientContext context;

            InitParameters request;
            request.ParseFromArray(packedRequest, (int)requestLen);
            InitResult initResult;
            auto Status = grpc::internal::BlockingUnaryCall(_inProcServer.get(), method, &context, request, &initResult);
            
            initResult.SerializeToArray(sideband_memory, 4096);
            //SetEvent(_callCompleteEvent);
            SignalReady();
        }
    }
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunSharedMemoryListener()
{
    SharedMemoryListener listener;
    listener.Run();
}
    
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunServer(const std::string& certPath, const char* server_address)
{
    // Init gRPC
    //grpc_init();
    //grpc_timer_manager_set_threading(false);
    // grpc_core::Executor::SetThreadingDefault(false);
    // grpc_core::Executor::SetThreadingAll(false);

	auto creds = CreateCredentials(certPath);

	NIPerfTestServer service;
    NIMonikerServer monikerService;
	ServerBuilder builder;
	builder.AddListeningPort(server_address, creds);
    builder.AddChannelArgument(GRPC_ARG_MINIMAL_STACK, 1);
	builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE);
	builder.SetMaxMessageSize(1 * 1024 * 1024);
	builder.SetMaxReceiveMessageSize(1 * 1024 * 1024);
    // GRPC_ARG_ENABLE_CHANNELZ
    // GRPC_ARG_ENABLE_CENSUS
    // GRPC_ARG_DISABLE_CLIENT_AUTHORITY_FILTER
    // GRPC_ARG_ENABLE_DEADLINE_CHECKS
    // GRPC_ARG_ENABLE_LOAD_REPORTING
    // GRPC_ARG_MINIMAL_STACK
    // GRPC_ARG_ENABLE_PER_MESSAGE_DECOMPRESSION
    // GRPC_ARG_ENABLE_RETRIES
    // GRPC_ARG_HTTP2_BDP_PROBE
    // GRPC_ARG_INHIBIT_HEALTH_CHECKING
    // GRPC_ARG_TCP_TX_ZEROCOPY_ENABLED
    // builder.AddChannelArgument();
	builder.RegisterService(&service);
	builder.RegisterService(&monikerService);

    auto cq = builder.AddCompletionQueue();

    std::cout << "Starting server" << std::endl;
	// Assemble the server.
	auto server = builder.BuildAndStart();
    if (_inProcServer == nullptr)
    {
        _inProcServer = server->InProcessChannel(grpc::ChannelArguments());
    }
	std::cout << "Server listening on " << server_address << std::endl;

    auto nextCall = new ReadComplexAsyncCall();
    service.RequestReadComplexArena(&nextCall->_context, &nextCall->_request, &nextCall->_response, cq.get(), cq.get(), nextCall);
    while (true)
    {
        bool ok;
        void* tag = nullptr;
        cq->Next(&tag, &ok); 

        auto call = reinterpret_cast<ReadComplexAsyncCall*>(tag);

        nextCall = new ReadComplexAsyncCall();
        service.RequestReadComplexArena(&nextCall->_context, &nextCall->_request, &nextCall->_response, cq.get(), cq.get(), nextCall);
        
        call->HandleCall(ok);
        //delete call;
    }
	server->Wait();
}

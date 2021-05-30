//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <thread>
#include <sstream>
#include <fstream>
#include <iostream>
#include <perftest_server.h>
#include <perftest.grpc.pb.h>
#include <src/core/lib/iomgr/executor.h>
#include <src/core/lib/iomgr/timer_manager.h>
#include <thread>

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

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;


//---------------------------------------------------------------------
//---------------------------------------------------------------------
using namespace std;
using namespace niPerfTest;


//---------------------------------------------------------------------
//---------------------------------------------------------------------
static ::grpc::ServerWriter< ::niPerfTest::StreamLatencyServer>* _writers [32];

//---------------------------------------------------------------------
//---------------------------------------------------------------------
::grpc::Status NIScopeServer::StreamLatencyTestClient(::grpc::ServerContext* context, ::grpc::ServerReader<::niPerfTest::StreamLatencyClient>* reader, ::niPerfTest::StreamLatencyServer* response)
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
        //cout << "Reader read at slot: " << slot << endl;
        if (_writers[slot] != nullptr)
        {
            //cout << "Reader writing to slot: " << slot << endl;
            _writers[slot]->Write(server);
        }
 	}
     _writers[slot] = nullptr;
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
::grpc::Status NIScopeServer::StreamLatencyTestServer(::grpc::ServerContext* context, const ::niPerfTest::StreamLatencyClient* request, ::grpc::ServerWriter< ::niPerfTest::StreamLatencyServer>* writer)
{
    auto slot = request->message();
    //cout << "Setting writer to slot: " << slot << endl;
    _writers[slot] = writer;
    while (_writers[slot] == writer)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    //cout << "Writer done at slot: " << slot << endl;
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::StreamLatencyTest(ServerContext* context, grpc::ServerReaderWriter<niPerfTest::StreamLatencyServer, niPerfTest::StreamLatencyClient>* stream)
{
	niPerfTest::StreamLatencyClient client;
	niPerfTest::StreamLatencyServer server;
	while (stream->Read(&client))
	{
		stream->Write(server);
	}
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::Init(ServerContext* context, const niPerfTest::InitParameters* request, niPerfTest::InitResult* response)
{
	response->set_status(request->id());
	return Status::OK;	
}


//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::InitWithOptions(ServerContext* context, const niPerfTest::InitWithOptionsParameters* request, niPerfTest::InitWithOptionsResult* response)
{	
	response->set_status(0);
	niPerfTest::ViSession* session = new niPerfTest::ViSession();
	session->set_id(1);
	response->set_allocated_newvi(session);
	response->set_status(0);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::Read(ServerContext* context, const niPerfTest::ReadParameters* request, niPerfTest::ReadResult* response)
{	
	response->mutable_wfm()->Reserve(request->numsamples());
	response->mutable_wfm()->Resize(request->numsamples(), 0.0);
	response->set_status(0);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::TestWrite(ServerContext* context, const niPerfTest::TestWriteParameters* request, niPerfTest::TestWriteResult* response)
{
	response->set_status(0);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::TestWriteContinuously(ServerContext* context, ::grpc::ServerReaderWriter<niPerfTest::TestWriteResult, niPerfTest::TestWriteParameters>* stream)
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
Status NIScopeServer::ReadContinuously(ServerContext* context, const niPerfTest::ReadContinuouslyParameters* request, grpc::ServerWriter<niPerfTest::ReadContinuouslyResult>* writer)
{			
	niPerfTest::ReadContinuouslyResult response;
	response.mutable_wfm()->Reserve(request->numsamples());
	response.mutable_wfm()->Resize(request->numsamples(), 0.0);
    auto iterations = request->numiterations();
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

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status MonikerServer::InitiateMonikerStream(::grpc::ServerContext* context, const ::niPerfTest::MonikerList* request, ::niPerfTest::MonikerStreamId* response)
{    
    response->set_streamid(1);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status MonikerServer::StreamReadWrite(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::niPerfTest::MonikerReadResult, ::niPerfTest::MonikerWriteRequest>* stream)
{    
	niPerfTest::MonikerWriteRequest client;
	while (stream->Read(&client))
	{
        niPerfTest::MonikerReadResult server;
        for (int x=0; x<client.values().size(); ++x)
        {
            niPerfTest::StreamLatencyClient writeValue;
            client.values()[x].UnpackTo(&writeValue);

            niPerfTest::StreamLatencyServer readValue;
            readValue.set_message(writeValue.message());
            auto newValue = server.add_values();
            newValue->PackFrom(readValue);
        }
		stream->Write(server);
	}
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
string GetServerAddress(int argc, char** argv)
{
    string target_str = "0.0.0.0:50051";
    string arg_str("--address");
    if (argc > 1)
    {
        string arg_val = argv[1];
        size_t start_pos = arg_val.find(arg_str);
        if (start_pos != string::npos)
        {
            start_pos += arg_str.size();
            if (arg_val[start_pos] == '=')
            {
                target_str = arg_val.substr(start_pos + 1);
            }
            else
            {
                cout << "The only correct argument syntax is --address=" << endl;
            }
        }
        else
        {
            cout << "The only acceptable argument is --address=" << endl;
        }
    }
    return target_str;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
string GetCertPath(int argc, char** argv)
{
    string cert_str;
    string arg_str("--cert");
    if (argc > 2)
    {
        string arg_val = argv[2];
        size_t start_pos = arg_val.find(arg_str);
        if (start_pos != string::npos)
        {
            start_pos += arg_str.size();
            if (arg_val[start_pos] == '=')
            {
                cert_str = arg_val.substr(start_pos + 1);
            }
            else
            {
                cout << "The only correct argument syntax is --cert=" << endl;
                return 0;
            }
        }
        else
        {
            cout << "The only acceptable argument is --cert=" << endl;
            return 0;
        }
    }
    return cert_str;
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
std::shared_ptr<grpc::ServerCredentials> CreateCredentials(int argc, char **argv)
{
	auto certPath = GetCertPath(argc, argv);

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

std::shared_ptr<grpc::Channel> _inProcServer;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunServer(int argc, char **argv, const char* saddress)
{
    grpc_init();
    grpc_timer_manager_set_threading(false);
    // ::grpc_core::Executor::SetThreadingDefault(false);
    // ::grpc_core::Executor::SetThreadingAll(false);

	auto server_address = saddress; //GetServerAddress(argc, argv);
	auto creds = CreateCredentials(argc, argv);

	NIScopeServer service;
    MonikerServer monikerService;
	grpc::EnableDefaultHealthCheckService(true);
	grpc::reflection::InitProtoReflectionServerBuilderPlugin();

	ServerBuilder builder;
	builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE);
	// Listen on the given address without any authentication mechanism.
	builder.AddListeningPort(server_address, creds);
	builder.SetMaxMessageSize(4 * 1024 * 1024);
	builder.SetMaxReceiveMessageSize(4 * 1024 * 1024);
    
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

	// Assemble the server.
	auto server = builder.BuildAndStart();
	_inProcServer = server->InProcessChannel(grpc_impl::ChannelArguments());
	cout << "Server listening on " << server_address << endl;
	server->Wait();
}

using timeVector = vector<chrono::microseconds>;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int main(int argc, char **argv)
{
#ifndef _WIN32    
    sched_param schedParam;
    schedParam.sched_priority = 95;
    sched_setscheduler(0, SCHED_FIFO, &schedParam);

    // cpu_set_t cpuSet;
    // CPU_ZERO(&cpuSet);
    // CPU_SET(1, &cpuSet);
    // sched_setaffinity(0, sizeof(cpu_set_t), &cpuSet);
#endif

    // std::vector<thread*> threads;
    // std::vector<string> ports;
    // for (int x=0; x<20; ++x)
    // {
    //     auto port = 50051 + x;
    //     auto portStr = string("0.0.0.0:") + to_string(port);
    //     ports.push_back(portStr);
    // } 
    // for (auto port: ports)
    // {
    //     auto p = new string(port.c_str());
    // 	auto t = new std::thread(RunServer, argc, argv, p->c_str());
    //     threads.push_back(t);
    // }
	// std::this_thread::sleep_for(std::chrono::milliseconds(500));
	// auto scope = NIScope(_inProcServer);
	// PerformLatencyStreamTest2(scope, "inprocess.txt");
    // for (auto t: threads)
    // {
    //     t->join();
    // }

    RunServer(argc, argv, "0.0.0.0:50051");
	return 0;
}
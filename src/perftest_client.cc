//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <client_utilities.h>
#include <cxxopts.hpp>
#include <performance_tests.h>
#include <test_config.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <perftest.grpc.pb.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <thread>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#ifndef _WIN32
#include <sched.h>
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using std::cout;
using std::endl;

using namespace std;
using namespace niPerfTest;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
string read_keycert(const string& filename)
{
    string data;
    ifstream file(filename.c_str(), ios::in);
    if (file.is_open())
    {
        stringstream ss;
        ss << file.rdbuf();
        file.close();
        data = ss.str();
    }
    return data;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
shared_ptr<grpc::ChannelCredentials> CreateCredentials(const string &certificatePath)
{
    shared_ptr<grpc::ChannelCredentials> creds;
    if (!certificatePath.empty())
    {
        string cacert = read_keycert(certificatePath);
        grpc::SslCredentialsOptions ssl_opts;
        ssl_opts.pem_root_certs=cacert;
        creds = grpc::SslCredentials(ssl_opts);
    }
    else
    {
        creds = grpc::InsecureChannelCredentials();
    }
    return creds;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunLatencyStreamTestSuite(NIPerfTestClient& client)
{
    cout << "Start Latency Stream Test Suite" << endl;
    EnableTracing();
    PerformLatencyStreamTest(client, "streamlatency1.txt");
    PerformLatencyStreamTest(client, "streamlatency2.txt");
    PerformLatencyStreamTest(client, "streamlatency3.txt");
    PerformLatencyStreamTest(client, "streamlatency4.txt");
    PerformLatencyStreamTest(client, "streamlatency5.txt");
    DisableTracing();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunMessageLatencyTestSuite(NIPerfTestClient& client)
{
    cout << "Start Message Latency Test Suite" << endl;
    PerformMessageLatencyTest(client, "latency1.txt");
    PerformMessageLatencyTest(client, "latency2.txt");
    PerformMessageLatencyTest(client, "latency3.txt");
    PerformMessageLatencyTest(client, "latency4.txt");
    PerformMessageLatencyTest(client, "latency5.txt");
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunLatencyPayloadWriteTestSuite(NIPerfTestClient& client)
{
    cout << "Start Latency Payload Write Test Suite" << endl;
    PerformLatencyPayloadWriteTest(client, 1, "payloadlatency1.txt");
    PerformLatencyPayloadWriteTest(client, 8, "payloadlatency8.txt");
    PerformLatencyPayloadWriteTest(client, 16, "payloadlatency16.txt");
    PerformLatencyPayloadWriteTest(client, 32, "payloadlatency32.txt");
    PerformLatencyPayloadWriteTest(client, 64, "payloadlatency64.txt");
    PerformLatencyPayloadWriteTest(client, 128, "payloadlatency128.txt");
    PerformLatencyPayloadWriteTest(client, 1024, "payloadlatency1024.txt");
    PerformLatencyPayloadWriteTest(client, 32768, "payloadlatency32768.txt");
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunLatencyPayloadWriteStreamTestSuite(NIPerfTestClient& client)
{
    cout << "Start Latency Payload Write Stream Test Suite" << endl;
    PerformLatencyPayloadWriteStreamTest(client, 1, "payloadstreamlatency1.txt");
    PerformLatencyPayloadWriteStreamTest(client, 8, "payloadstreamlatency8.txt");
    PerformLatencyPayloadWriteStreamTest(client, 16, "payloadstreamlatency16.txt");
    PerformLatencyPayloadWriteStreamTest(client, 32, "payloadstreamlatency32.txt");
    PerformLatencyPayloadWriteStreamTest(client, 64, "payloadstreamlatency64.txt");
    PerformLatencyPayloadWriteStreamTest(client, 128, "payloadstreamlatency128.txt");
    PerformLatencyPayloadWriteStreamTest(client, 1024, "payloadstreamlatency1024.txt");
    PerformLatencyPayloadWriteStreamTest(client, 32768, "payloadstreamlatency32768.txt");
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunParallelStreamTest(int numClients, const string& channelTarget, std::shared_ptr<grpc::ChannelCredentials> creds)
{
    cout << "Start Parallel Stream Test Suite" << endl;
    std::vector<NIPerfTestClient*> clients;
    for (int x=0; x<numClients; ++x)
    {
        // Set a dummy (but distinct) channel arg on each channel so that
        // every channel gets its own connection
        grpc::ChannelArguments args;
        args.SetInt(GRPC_ARG_MINIMAL_STACK, 1);
        args.SetInt("ClientIndex", x);
        auto client = new NIPerfTestClient(grpc::CreateCustomChannel(channelTarget, creds, args));
        clients.push_back(client);
    }
    cout << endl << "Start " << numClients << " streaming tests" << endl;
    PerformNStreamTest(clients, 1000);
    PerformNStreamTest(clients, 10000);
    PerformNStreamTest(clients, 100000);
    PerformNStreamTest(clients, 200000);
    //PerformNStreamTest(clients, 400000);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunParallelStreamTestSuite(const string& channelTarget, std::shared_ptr<grpc::ChannelCredentials> creds)
{
    RunParallelStreamTest(2, channelTarget, creds);
    RunParallelStreamTest(4, channelTarget, creds);
    RunParallelStreamTest(8, channelTarget, creds);
    RunParallelStreamTest(16, channelTarget, creds);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunParallelatencyStreamTestSuite(NIPerfTestClient& client)
{
    cout << "Start Parallel Stream Latency Test Suite" << endl;
    PerformLatencyStreamTest2(client, client, 1, "streamlatency1Stream.txt");
    PerformLatencyStreamTest2(client, client, 2, "streamlatency1Stream.txt");
    PerformLatencyStreamTest2(client, client, 3, "streamlatency1Stream.txt");
    PerformLatencyStreamTest2(client, client, 4, "streamlatency4Stream.txt");
    PerformLatencyStreamTest2(client, client, 5, "streamlatency4Stream.txt");
}
    
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunMessagePerformanceTestSuite(NIPerfTestClient& client)
{
    cout << "Start Message Performance Test Suite" << endl;
    PerformMessagePerformanceTest(client);
}
    
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunSteamingTestSuite(NIPerfTestClient& client)
{
    cout << "Start Streaming Test Suite" << endl;
    PerformStreamingTest(client, 10);
    PerformStreamingTest(client, 100);
    PerformStreamingTest(client, 1000);
    PerformStreamingTest(client, 10000);
    PerformStreamingTest(client, 100000);
    // PerformStreamingTest(client, 100000);
    // PerformStreamingTest(client, 100000);
    // PerformStreamingTest(client, 100000);
    PerformStreamingTest(client, 200000);
    //  PerformStreamingTest(client, 400000);
    //  PerformStreamingTest(client, 1000000);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunReadTestSuite(NIPerfTestClient& client)
{
    cout << "Start Read Test Suite" << endl;
    PerformReadTest(client, 100, 1000);
    PerformReadTest(client, 1000, 1000);
    PerformReadTest(client, 10000, 1000);
    PerformReadTest(client, 100000, 1000);
    PerformReadTest(client, 800000, 1000);
    // PerformReadTest(client, 200000, 10000);
    // PerformReadTest(client, 393216, 10000);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunReadComplexTestSuite(NIPerfTestClient& client)
{
    cout << "Start Read Complex Test Suite" << endl;
    PerformReadComplexArenaTest(client, 100, 1000);
    PerformReadComplexArenaTest(client, 1000, 1000);
    PerformReadComplexArenaTest(client, 10000, 1000);
    PerformReadComplexArenaTest(client, 100000, 1000);

    PerformReadComplexTest(client, 100, 1000);
    PerformReadComplexTest(client, 1000, 1000);
    PerformReadComplexTest(client, 10000, 1000);
    PerformReadComplexTest(client, 100000, 1000);
    //PerformReadComplexTest(client, 400000, 1000);
    // PerformReadTest(client, 200000, 10000);
    // PerformReadTest(client, 393216, 10000);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunWriteTestSuite(NIPerfTestClient& client)
{
    cout << "Start Write Test Suite" << endl;
    PerformWriteTest(client, 100);
    PerformWriteTest(client, 1000);
    PerformWriteTest(client, 10000);
    PerformWriteTest(client, 100000);
    PerformWriteTest(client, 200000);
    PerformWriteTest(client, 393216);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunScpiCompareTestSuite(NIPerfTestClient& client)
{
    cout << "Start SCPI Compare Test Suite" << endl;
    // PerformReadTest(client, 10, 100000);
    // PerformAsyncInitTest(client, 10, 10000);
    PerformScopeLikeRead(client);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#if ENABLE_GRPC_SIDEBAND
void RunSidebandDataTestSuite(NIPerfTestClient& client)
{
    cout << "Start Sideband Data Test Suite" << endl << endl;

    // cout << "Single Buffer, Standard memcpy" << endl;
    // PerformSidebandReadTest(client, 1024 * 1024, SidebandStrategy::SHARED_MEMORY, false, "1 MB Buffer");
    // PerformSidebandReadTest(client, 2 * 1024 * 1024, SidebandStrategy::SHARED_MEMORY, false, "2 MB Buffer");
    // PerformSidebandReadTest(client, 4 * 1024 * 1024, SidebandStrategy::SHARED_MEMORY, false, "4 MB Buffer");
    // PerformSidebandReadTest(client, 8 * 1024 * 1024, SidebandStrategy::SHARED_MEMORY, false, "8 MB Buffer");
    // PerformSidebandReadTest(client, 12 * 1024 * 1024, SidebandStrategy::SHARED_MEMORY, false, "12 MB Buffer");
    // PerformSidebandReadTest(client, 16 * 1024 * 1024, SidebandStrategy::SHARED_MEMORY, false, "16 MB Buffer");

    // cout << endl;
    // cout << "Double Buffer, Standard memcpy" << endl;
    // PerformSidebandReadTest(client, 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, false, "1 MB Buffer");
    // PerformSidebandReadTest(client, 2 * 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, false, "2 MB Buffer");
    // PerformSidebandReadTest(client, 4 * 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, false, "4 MB Buffer");
    // PerformSidebandReadTest(client, 8 * 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, false, "8 MB Buffer");
    // PerformSidebandReadTest(client, 12 * 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, false, "12 MB Buffer");
    // PerformSidebandReadTest(client, 16 * 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, false, "16 MB Buffer");

    // cout << endl;
    // cout << "Single Buffer, Fast memcpy" << endl;
    // PerformSidebandReadTest(client, 1024 * 1024, SidebandStrategy::SHARED_MEMORY, true, "1 MB Buffer");
    // PerformSidebandReadTest(client, 2 * 1024 * 1024, SidebandStrategy::SHARED_MEMORY, true, "2 MB Buffer");
    // PerformSidebandReadTest(client, 4 * 1024 * 1024, SidebandStrategy::SHARED_MEMORY, true, "4 MB Buffer");
    // PerformSidebandReadTest(client, 8 * 1024 * 1024, SidebandStrategy::SHARED_MEMORY, true, "8 MB Buffer");
    // PerformSidebandReadTest(client, 12 * 1024 * 1024, SidebandStrategy::SHARED_MEMORY, true, "12 MB Buffer");
    // PerformSidebandReadTest(client, 16 * 1024 * 1024, SidebandStrategy::SHARED_MEMORY, true, "16 MB Buffer");

    // cout << endl;
    // cout << "Double Buffer, Fast memcpy" << endl;
    // PerformSidebandReadTest(client, 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, true, "1 MB Buffer");
    // PerformSidebandReadTest(client, 2 * 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, true, "2 MB Buffer");
    // PerformSidebandReadTest(client, 4 * 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, true, "4 MB Buffer");
    // PerformSidebandReadTest(client, 8 * 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, true, "8 MB Buffer");
    // PerformSidebandReadTest(client, 12 * 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, true, "12 MB Buffer");
    // PerformSidebandReadTest(client, 16 * 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, true, "16 MB Buffer");

    // cout << endl;
    cout << "Sockets" << endl;
    // PerformSidebandReadTest(client, 1024 * 1024, SidebandStrategy::SOCKETS, true, "1 MB Buffer");
    // PerformSidebandReadTest(client, 2 * 1024 * 1024, SidebandStrategy::SOCKETS, true, "2 MB Buffer");
    PerformSidebandReadTest(client, 4 * 1024 * 1024, SidebandStrategy::SOCKETS, "4 MB Buffer");
    // PerformSidebandReadTest(client, 8 * 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, true, "8 MB Buffer");
    // PerformSidebandReadTest(client, 12 * 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, true, "12 MB Buffer");
    // PerformSidebandReadTest(client, 16 * 1024 * 1024, SidebandStrategy::DOUBLE_BUFFERED_SHARED_MEMORY, true, "16 MB Buffer");

    cout << "RDMA" << endl;
    PerformSidebandReadTest(client, 4 * 1024 * 1024, SidebandStrategy::RDMA, "4 MB Buffer");

    cout << endl;
    cout << "Done" << endl;
}
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformPackingTests();

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunConfiguredTests(const TestConfig& config, NIPerfTestClient& client, 
                        const string& channelTarget, std::shared_ptr<grpc::ChannelCredentials> creds,
                        MonikerClient* monikerClient = nullptr, NIPerfTestClient* udsClient = nullptr)
{
    cout << endl << "Running configured tests..." << endl;

    // Run test suites
    if (config.message_performance.enabled)
    {
        RunMessagePerformanceTestSuite(client);
    }

    if (config.latency_stream.enabled)
    {
        RunLatencyStreamTestSuite(client);
    }

    if (config.message_latency.enabled)
    {
        RunMessageLatencyTestSuite(client);
    }

    if (config.read_tests.enabled)
    {
        RunReadTestSuite(client);
    }

    if (config.read_complex_tests.enabled)
    {
        RunReadComplexTestSuite(client);
    }

    if (config.write_tests.enabled)
    {
        RunWriteTestSuite(client);
    }

    if (config.streaming_tests.enabled)
    {
        RunSteamingTestSuite(client);
    }

    if (config.scpi_compare_tests.enabled)
    {
        RunScpiCompareTestSuite(client);
    }

    // Run UDS tests if enabled and client available
#if ENABLE_UDS_TESTS
    if (config.uds_tests.enabled && udsClient)
    {
        cout << "UDS TESTS" << endl;
        RunMessagePerformanceTestSuite(*udsClient);
        RunLatencyStreamTestSuite(*udsClient);
    }
#endif

    // Run single latency stream test
    if (config.single_latency_stream.enabled)
    {
        PerformLatencyStreamTest(client, config.single_latency_stream.filename);
    }

    // Run async init tests
    if (config.async_init_tests.enabled)
    {
        cout << "Running Async Init Tests" << endl;
        for (int commands : config.async_init_tests.command_counts)
        {
            PerformAsyncInitTest(client, commands, config.async_init_tests.iterations);
        }
    }

    // Run parallel stream tests
    if (config.parallel_stream_tests.enabled)
    {
        RunParallelStreamTestSuite(channelTarget, creds);
    }

    // Run payload write tests
    if (config.payload_write_tests.enabled)
    {
        cout << "Start Configured Payload Write Tests" << endl;
        for (int size : config.payload_write_tests.payload_sizes)
        {
            string filename = config.payload_write_tests.output_prefix + to_string(size) + ".txt";
            PerformLatencyPayloadWriteTest(client, size, filename);
        }
    }

    // Run payload stream tests
    if (config.payload_stream_tests.enabled)
    {
        cout << "Start Configured Payload Stream Tests" << endl;
        for (int size : config.payload_stream_tests.payload_sizes)
        {
            string filename = config.payload_stream_tests.output_prefix + to_string(size) + ".txt";
            PerformLatencyPayloadWriteStreamTest(client, size, filename);
        }
    }

#if ENABLE_GRPC_SIDEBAND
    // Run sideband tests
    if (config.sideband_tests.enabled)
    {
        cout << "Start Configured Sideband Tests" << endl;
        for (const string& strategy_str : config.sideband_tests.strategies)
        {
            cout << strategy_str << " Tests" << endl;
            for (int buffer_size : config.sideband_tests.buffer_sizes)
            {
                string message = to_string(buffer_size / (1024 * 1024)) + " MB Buffer";
                
                if (strategy_str == "SOCKETS")
                {
                    PerformSidebandReadTest(client, buffer_size, ni::data_monikers::SidebandStrategy::SOCKETS, message);
                }
                else if (strategy_str == "RDMA")
                {
                    PerformSidebandReadTest(client, buffer_size, ni::data_monikers::SidebandStrategy::RDMA, message);
                }
                // Add more strategies as needed
            }
        }
        
        // Run sideband moniker tests if available
        if (monikerClient)
        {
            for (const string& strategy_str : config.sideband_tests.strategies)
            {
                if (strategy_str == "SOCKETS_LOW_LATENCY")
                {
                    PerformSidebandMonikerLatencyTest(*monikerClient, 1, ni::data_monikers::SidebandStrategy::SOCKETS_LOW_LATENCY);
                }
                else if (strategy_str == "RDMA_LOW_LATENCY")
                {
                    PerformSidebandMonikerLatencyTest(*monikerClient, 1, ni::data_monikers::SidebandStrategy::RDMA_LOW_LATENCY);
                }
            }
        }
    }
#endif

    // Run packing tests
    if (config.packing_tests.enabled)
    {
        cout << "Start Configured Packing Tests" << endl;
        PerformPackingTests();
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformPackingTests()
{
#if (ENABLE_FLATBUFFERS)
    PerformFlatbuffersPackLidarTest(1000, 10000);
    PerformFlatbuffersPackLidarTest(10000, 10000);
    PerformFlatbuffersPackLidarTest(1, 5000);
    PerformFlatbuffersPackLidarTest(10, 5000);
    PerformFlatbuffersPackLidarTest(100, 5000);
    PerformFlatbuffersPackLidarTest(1000, 5000);
    PerformFlatbuffersPackLidarTest(10000, 5000);
    PerformFlatbuffersPackLidarTest(50000, 5000);
    PerformFlatbuffersPackLidarTest(100000, 5000);

    PerformFlatbuffersPackUnpackLidarTest(1, 5000);
    PerformFlatbuffersPackUnpackLidarTest(10, 5000);
    PerformFlatbuffersPackUnpackLidarTest(100, 5000);
    PerformFlatbuffersPackUnpackLidarTest(1000, 5000);
    PerformFlatbuffersPackUnpackLidarTest(10000, 5000);
    PerformFlatbuffersPackUnpackLidarTest(50000, 5000);
    PerformFlatbuffersPackUnpackLidarTest(100000, 5000);
    PerformFlatbuffersPackUnpackLidarTest(200000, 5000);

    PerformFlatbuffersPackUnpackTest(100000, 100);
    PerformFlatbuffersPackUnpackTest(200000, 100);
#endif
    PerformArenaPackLidarVectorsTest(1000, 10000);
    PerformArenaPackLidarVectorsTest(10000, 10000);
    PerformArenaPackLidarVectorsTest(100000, 10000);

    PerformArenaPackUnpackLidarVectorsTest(1000, 10000);
    PerformArenaPackUnpackLidarVectorsTest(10000, 10000);
    PerformArenaPackUnpackLidarVectorsTest(100000, 100);

    PerformArenaPackLidarTest(1000, 10000);
    PerformArenaPackLidarTest(10000, 10000);
    PerformArenaPackLidarTest(1, 5000);
    PerformArenaPackLidarTest(10, 5000);
    PerformArenaPackLidarTest(100, 5000);
    PerformArenaPackLidarTest(1000, 5000);
    PerformArenaPackLidarTest(10000, 5000);
    PerformArenaPackLidarTest(100000, 5000);

    PerformArenaPackUnpackLidarTest(1000, 10000);
    PerformArenaPackUnpackLidarTest(10000, 10000);
    PerformArenaPackUnpackLidarTest(1, 5000);
    PerformArenaPackUnpackLidarTest(10, 5000);
    PerformArenaPackUnpackLidarTest(100, 5000);
    PerformArenaPackUnpackLidarTest(1000, 5000);
    PerformArenaPackUnpackLidarTest(10000, 5000);
    PerformArenaPackUnpackLidarTest(100000, 5000);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int main(int argc, char **argv)
{
    cxxopts::Options options("perftest_client", "gRPC client for testing various aspects of gRPC performance");
    options.add_options()
      ("c,cert", "path to the certificate file to be used", cxxopts::value<string>()->default_value(""))
      ("t,target", "target address of the desired server server", cxxopts::value<string>()->default_value("localhost"))
      ("p,port", "port to connect to on the target", cxxopts::value<int>()->default_value("50051"))
      ("config", "path to the test configuration file", cxxopts::value<string>()->default_value("test_config.json"))
      ("generate-config", "generate a default configuration file and exit")
      ("h,help", "show usage")
      ;

    auto parse_result = options.parse(argc, argv);
    if (parse_result.count("help"))
    {
        std::cout << options.help() << std::endl;
        return 0;
    }

    // Handle config file generation
    if (parse_result.count("generate-config"))
    {
        TestConfig config;
        string config_file = parse_result["config"].as<string>();
        if (config.SaveDefaultToFile(config_file))
        {
            cout << "Default configuration saved to: " << config_file << endl;
        }
        else
        {
            cout << "Failed to save configuration file!" << endl;
            return -1;
        }
        return 0;
    }

    string cert_path = parse_result["cert"].as<string>();
    string target = parse_result["target"].as<string>();
    int port = parse_result["port"].as<int>();
    string config_file = parse_result["config"].as<string>();

    // Load test configuration
    TestConfig test_config;
    test_config.LoadFromFile(config_file);

    // Configure gRPC
    // grpc_init();
    // grpc_timer_manager_set_threading(false);
    // ::grpc_core::Executor::SetThreadingDefault(false);
    // ::grpc_core::Executor::SetThreadingAll(false);

    // Configure enviornment
#ifndef _WIN32    
    sched_param schedParam;
    schedParam.sched_priority = 99;
    sched_setscheduler(0, SCHED_FIFO, &schedParam);

    // cpu_set_t cpuSet;
    // CPU_ZERO(&cpuSet);
    // CPU_SET(4, &cpuSet);
    // //CPU_SET(10, &cpuSet);
    // sched_setaffinity(0, sizeof(cpu_set_t), &cpuSet);
#else
    DWORD dwError, dwPriClass;
    if(!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
    {
        dwError = GetLastError();
        if( ERROR_PROCESS_MODE_ALREADY_BACKGROUND == dwError)
            cout << "Already in background mode" << endl;
        else
            cout << "Failed change priority: " << dwError << endl;
   } 
#endif

    auto creds = CreateCredentials(cert_path);
    cout << "Target: " << target << " Port: " << port << endl;

    // Create the connection to the server
    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_MINIMAL_STACK, 1);
    args.SetMaxReceiveMessageSize(10 * 100 * 1024 * 1024);
    args.SetMaxSendMessageSize(10 * 100 * 1024 * 1024);
#if ENABLE_UDS_TESTS
    auto udsClient = new NIPerfTestClient(grpc::CreateCustomChannel("unix:///tmp/perftest", creds, args));
#endif
    string channelTarget = target + ":" + to_string(port);
    auto client = new NIPerfTestClient(grpc::CreateCustomChannel(channelTarget, creds, args));
    auto monikerClient = new MonikerClient(grpc::CreateCustomChannel(channelTarget, creds, args));

    // Verify the client is working correctly
    auto result = client->Init(42);
    if (result != 42)
    {
        cout << "Server communication failure!" << endl;
        return -1;
    }
    else
    {
        cout << "Init result: " << result << endl;
    }
    // Verify the client is working correctly
#if ENABLE_UDS_TESTS
    result = udsClient->Init(42);
    if (result != 42)
    {
        cout << "Server UDS communication failure!" << endl;
        return -1;
    }
    else
    {
        cout << "Init result: " << result << endl;
    }
#endif

    // Run desired test suites based on configuration
    RunConfiguredTests(test_config, *client, channelTarget, creds, monikerClient
#if ENABLE_UDS_TESTS
        , udsClient
#else
        , nullptr
#endif
    );
    
    return 0;   
}

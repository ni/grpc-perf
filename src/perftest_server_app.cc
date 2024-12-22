//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <client_utilities.h>

#if ENABLE_GRPC_SIDEBAND
#include <sideband_data.h>
#include <sideband_grpc.h>
#endif

#include <performance_tests.h>
#include <cxxopts.hpp>
#include <thread>
#include <sstream>
#include <fstream>
#include <iostream>
#include <perftest_server.h>
#include <perftest.grpc.pb.h>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sched.h>
#endif

void InitDetours();
void RunServer(const std::string& certPath, const char* server_address);

#if ENABLE_GRPC_SIDEBAND
void RunAccept()
{
    std::atomic_bool stop;
    RunSidebandSocketsAccept("localhost", 50055, stop);
}
#endif

int main(int argc, char **argv)
{
    cxxopts::Options options("perftest_server", "gRPC server for testing various aspects of gRPC performance");
    options.add_options()
      ("c,cert", "path to the certificate file to be used", cxxopts::value<std::string>()->default_value(""))
      ("h,help", "show usage")
      ;

    auto parse_result = options.parse(argc, argv);
    if (parse_result.count("help"))
    {
        std::cout << options.help() << std::endl;
        return 0;
    }

    std::string certPath = parse_result["cert"].as<std::string>();

    //InitDetours();
    // grpc_init();
    // grpc_timer_manager_set_threading(false);
    // grpc_core::Executor::SetThreadingDefault(false);
    // grpc_core::Executor::SetThreadingAll(false);

#ifndef _WIN32    
    sched_param schedParam;
    schedParam.sched_priority = 95;
    sched_setscheduler(0, SCHED_FIFO, &schedParam);

    // cpu_set_t cpuSet;
    // CPU_ZERO(&cpuSet);
    // CPU_SET(9, &cpuSet);
    // //CPU_SET(11, &cpuSet);
    // sched_setaffinity(1, sizeof(cpu_set_t), &cpuSet);
#else
    if(!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
    {
        auto dwError = GetLastError();
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
        auto port = 50051 + x;
        auto portStr = std::string("0.0.0.0:") + std::to_string(port);
        ports.push_back(portStr);
    } 
    for (auto port: ports)
    {
        auto p = new std::string(port.c_str());
        auto t = new std::thread(RunServer, certPath, p->c_str());
        threads.push_back(t);
    }

#if ENABLE_UDS_TESTS
    auto udsT = new std::thread(RunServer, certPath, "unix:///tmp/perftest");
    threads.push_back(udsT);
#endif

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

#if ENABLE_GRPC_SIDEBAND
    auto t = new std::thread(RunAccept);
    threads.push_back(t);

    auto t2 = new std::thread(AcceptSidebandRdmaReceiveRequests);
    threads.push_back(t2);
    auto t3 = new std::thread(AcceptSidebandRdmaSendRequests);
    threads.push_back(t3);
#endif

    // localhost testing
    //{
    //    auto target_str = std::string("localhost");
    //    auto creds = grpc::InsecureChannelCredentials();
    //    auto port = ":50051";
    //    ::grpc::ChannelArguments args;
    //    args.SetInt(GRPC_ARG_MINIMAL_STACK, 1);
    //    //auto client = new NIPerfTestClient(grpc::CreateCustomChannel(target_str + port, creds, args));
    //    //auto client = new NIPerfTestClient(grpc::CreateCustomChannel(target_str + port, creds, args));

    //    // inprocess server
    //    auto client = new NIPerfTestClient(_inProcServer);

    //    auto result = client->Init(42);
    //    cout << "Init result: " << result << endl;
    //    result = client->Init(43);
    //    cout << "Init result: " << result << endl;
    //    result = client->Init(44);
    //    cout << "Init result: " << result << endl;

    //    cout << "Start streaming tests" << endl;
    //    PerformStreamingTest(*client, 100000);

    //    PerformLatencyStreamTest(*client, "streamlatency1.txt");
    //    cout << "Performing streaming test" << endl;
    //    PerformStreamingTest(*client, 100000);
    //}

    // {
    //     auto target_str = std::string("localhost");
    //     auto creds = grpc::InsecureChannelCredentials();
    //     auto port = ":50051";
    //     ::grpc::ChannelArguments args;
    //     args.SetInt(GRPC_ARG_MINIMAL_STACK, 1);
    //     auto client = new NIPerfTestClient(grpc::CreateCustomChannel(target_str + port, creds, args));
    //     // auto client = new NIPerfTestClient(grpc::CreateCustomChannel(target_str + port, creds, args));

    //     // inprocess server
    //     //auto client = new NIPerfTestClient(_inProcServer);

    //     auto result = client->Init(42);
    //     cout << "Init result: " << result << endl;
    //     result = client->Init(43);
    //     cout << "Init result: " << result << endl;
    //     result = client->Init(44);
    //     cout << "Init result: " << result << endl;

    //     cout << "Start streaming tests" << endl;
    //     PerformStreamingTest(*client, 200000);

    //     //PerformLatencyStreamTest(*client, "streamlatency1.txt");
    //     //cout << "Performing streaming test" << endl;
    //     PerformStreamingTest(*client, 100000);
    // }
    
    for (auto t: threads)
    {
        t->join();
    }
	return 0;
}

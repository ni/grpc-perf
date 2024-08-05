//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <cxxopts.hpp>
#include <client_utilities.h>
#include <sideband_data.h>
#include <sideband_grpc.h>
#include <performance_tests.h>
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
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sched.h>
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int main(int argc, char **argv)
{
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
        auto t = new std::thread(RunServer, std::string(), p->c_str());
        threads.push_back(t);
    }

#if ENABLE_UDS_TESTS
    auto udsT = new std::thread(RunServer, 0, argv, "unix:///tmp/perftest");
    threads.push_back(udsT);
#endif

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

#if ENABLE_GRPC_SIDEBAND
    auto t = new std::thread(RunSidebandSocketsAccept, "localhost", 50055);
    threads.push_back(t);

    auto t2 = new std::thread(AcceptSidebandRdmaReceiveRequests);
    threads.push_back(t2);
    auto t3 = new std::thread(AcceptSidebandRdmaSendRequests);
    threads.push_back(t3);
#endif

    {
        auto target_str = std::string("localhost");
        auto creds = grpc::InsecureChannelCredentials();
        auto port = ":50051";
        ::grpc::ChannelArguments args;
        args.SetInt(GRPC_ARG_MINIMAL_STACK, 1);
        auto client = new NIPerfTestClient(grpc::CreateCustomChannel(target_str + port, creds, args));
        auto result = client->Init(42);
        std::cout << "Init result: " << result << std::endl;

        while (true)
        {
            PerformMessagePerformanceTest(*client);
            PerformLatencyStreamTest(*client, "localhostperf.txt");
            PerformAsyncInitTest(*client, 10, 10000);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    
    for (auto t: threads)
    {
        t->join();
    }
	return 0;
}

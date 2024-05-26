//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <client_utilities.h>
#include <sideband_data.h>
#include <sideband_grpc.h>
#include <performance_tests.h>
#include <cxxopts.hpp>
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

int main(int argc, char **argv)
{
    InitDetours();
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

    std::cout << "Start" << std::endl;

    auto p = new std::string("0.0.0.0:50051");
    auto serverThread = new std::thread(RunServer, std::string(), p->c_str());
    std::cout << "Server started" << std::endl;

    while (InProcServer() == nullptr)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    while (true)
    {
        {
            auto client = new NIPerfTestClient(InProcServer());
            std::cout << "InProcServer perf test" << std::endl;
            auto result = client->Init(42);
            std::cout << "Init result: " << result << std::endl;
            result = client->Init(43);
            std::cout << "Init result: " << result << std::endl;
            result = client->Init(44);
            std::cout << "Init result: " << result << std::endl;

            PerformMessagePerformanceTest(*client);
        }

        {
            auto target_str = std::string("localhost");
            auto creds = grpc::InsecureChannelCredentials();
            auto port = ":50051";
            ::grpc::ChannelArguments args;
            args.SetInt(GRPC_ARG_MINIMAL_STACK, 1);
            auto client = new NIPerfTestClient(grpc::CreateCustomChannel(target_str + port, creds, args));
            std::cout << "User mode networking perf test" << std::endl;
            auto result = client->Init(42);
            std::cout << "Init result: " << result << std::endl;
            result = client->Init(43);
            std::cout << "Init result: " << result << std::endl;
            result = client->Init(44);
            std::cout << "Init result: " << result << std::endl;

            PerformMessagePerformanceTest(*client);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
	return 0;
}

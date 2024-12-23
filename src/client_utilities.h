//---------------------------------------------------------------------
//---------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <grpcpp/grpcpp.h>
#include <perftest.grpc.pb.h>
#include <data_moniker.grpc.pb.h>
#include <memory>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using namespace niPerfTest;
using namespace ni::data_monikers;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using timeVector = std::vector<std::chrono::microseconds>;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
struct AsyncInitResults
{
    Status status;
    ClientContext context;
    InitResult reply;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class NIPerfTestClient
{
public:
    NIPerfTestClient(std::shared_ptr<Channel> channel);

public:
    int Init(int id);
    int Init(int id, std::string command);
    int InitAsync(int id, std::string command, grpc::CompletionQueue& cq,  AsyncInitResults* results);
    int ConfigureVertical(std::string vi, std::string channelList, double range, double offset, VerticalCoupling coupling, double probe_attenuation, bool enabled);
    int ConfigureHorizontalTiming(std::string vi, double min_sample_rate, int min_num_pts, double ref_position, int num_records, bool enforce_realtime);
    int InitiateAcquisition(std::string vi);
    int Read(double timeout, int numSamples, double* samples);
    int ReadComplex(double timeout, int numSamples);
    int ReadComplexArena(double timeout, int numSamples);
    int TestWrite(int numSamples, double* samples);
    std::unique_ptr<grpc::ClientReader<niPerfTest::ReadContinuouslyResult>> ReadContinuously(grpc::ClientContext* context, double timeout, int numSamples, int numIterations);
public:
    std::unique_ptr<niPerfTestService::Stub> _stub;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class MonikerClient
{
public:
    MonikerClient(std::shared_ptr<Channel> channel);

public:
    std::unique_ptr<MonikerService::Stub> _stub;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void WriteLatencyData(timeVector times, const std::string& fileName);
void ReadSamples(NIPerfTestClient* client, int numSamples, int numIterations);
void ReportMBPerSecond(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end, int numSamples, int numIterations);
void EnableTracing();
void DisableTracing();
void TracingOff();
void TracingOn();
void TraceMarker(const char* marker);

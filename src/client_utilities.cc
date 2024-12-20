//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include "client_utilities.h"
#include <fstream>

using namespace std;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
NIPerfTestClient::NIPerfTestClient(shared_ptr<Channel> channel)
    : _stub(niPerfTestService::NewStub(channel))
{        
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIPerfTestClient::Init(int id)
{
    InitParameters request;
    request.set_id(id);

    ClientContext context;
    InitResult reply;
    Status status = _stub->Init(&context, request, &reply);
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIPerfTestClient::Init(int id, string command)
{
    InitParameters request;
    request.set_id(id);
    request.set_command(command);

    ClientContext context;
    InitResult reply;
    Status status = _stub->Init(&context, request, &reply);
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIPerfTestClient::InitAsync(int id, string command, grpc::CompletionQueue& cq,  AsyncInitResults* results)
{
    InitParameters request;
    request.set_id(id);
    request.set_command(command);
    
    auto rpc = _stub->PrepareAsyncInit(&results->context, request, &cq);
    rpc->StartCall();
    rpc->Finish(&results->reply, &results->status, (void*)results);
    return 0;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIPerfTestClient::ConfigureVertical(string vi, string channelList, double range, double offset, VerticalCoupling coupling, double probe_attenuation, bool enabled)
{    
    ConfigureVerticalRequest request;
    request.set_vi(vi);
    request.set_channel_list(channelList);
    request.set_range(range);
    request.set_offset(offset);
    request.set_coupling(coupling);
    request.set_probe_attenuation(probe_attenuation);
    request.set_enabled(enabled);

    ClientContext context;
    ConfigureVerticalResponse reply;
    Status status = _stub->ConfigureVertical(&context, request, &reply);
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIPerfTestClient::ConfigureHorizontalTiming(string vi, double min_sample_rate, int min_num_pts, double ref_position, int num_records, bool enforce_realtime)
{    
    ConfigureHorizontalTimingRequest request;
    request.set_vi(vi);
    request.set_min_sample_rate(min_sample_rate);
    request.set_min_num_pts(min_num_pts);
    request.set_ref_position(ref_position);
    request.set_num_records(num_records);
    request.set_enforce_realtime(enforce_realtime);

    ClientContext context;
    ConfigureHorizontalTimingResponse reply;
    Status status = _stub->ConfigureHorizontalTiming(&context, request, &reply);
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIPerfTestClient::InitiateAcquisition(string vi)
{    
    InitiateAcquisitionRequest request;
    request.set_vi(vi);

    ClientContext context;
    InitiateAcquisitionResponse reply;
    Status status = _stub->InitiateAcquisition(&context, request, &reply);
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIPerfTestClient::Read(double timeout, int numSamples, double* samples)
{
    ReadParameters request;
    request.set_timeout(timeout);
    request.set_num_samples(numSamples);

    ClientContext context;
    ReadResult reply;
    Status status = _stub->Read(&context, request, &reply);    
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    if (reply.samples().size() != numSamples)
    {
        cout << "ERROR, wrong number of samples";
    }
    memcpy(samples, reply.samples().data(), numSamples * sizeof(double));
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIPerfTestClient::ReadComplex(double timeout, int numSamples)
{
    ReadParameters request;
    request.set_timeout(timeout);
    request.set_num_samples(numSamples);

    ClientContext context;
    ReadComplexResult reply;
    Status status = _stub->ReadComplex(&context, request, &reply);    
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    if (reply.samples().size() != numSamples)
    {
        cout << "ERROR, wrong number of samples";
    }
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIPerfTestClient::ReadComplexArena(double timeout, int numSamples)
{
    ReadParameters request;
    request.set_timeout(timeout);
    request.set_num_samples(numSamples);

    ClientContext context;
    ReadComplexResult reply;
    Status status = _stub->ReadComplexArena(&context, request, &reply);    
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    if (reply.samples().size() != numSamples)
    {
        cout << "ERROR, wrong number of samples";
    }
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIPerfTestClient::TestWrite(int numSamples, double* samples)
{   
    TestWriteParameters request;
    request.mutable_samples()->Reserve(numSamples);
    request.mutable_samples()->Resize(numSamples, 0);

    ClientContext context;
    TestWriteResult reply;
    auto status = _stub->TestWrite(&context, request, &reply);
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
unique_ptr<grpc::ClientReader<niPerfTest::ReadContinuouslyResult>> NIPerfTestClient::ReadContinuously(grpc::ClientContext* context, double timeout, int numSamples, int numIterations)
{    
    ReadContinuouslyParameters request;
    request.set_num_samples(numSamples);
    request.set_num_iterations(numIterations);

    return _stub->ReadContinuously(context, request);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
MonikerClient::MonikerClient(std::shared_ptr<Channel> channel)
    : _stub(MonikerService::NewStub(channel))
{    
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void WriteLatencyData(timeVector times, const string& fileName)
{
    auto iterations = times.size();

    {
        std::ofstream fout;
        fout.open("xaxis");
        for (int x=0; x<iterations; ++x)
        {
            fout << (x+1) << std::endl;
        }
        fout.close();
    }

    {
        std::ofstream fout;
        fout.open(fileName);
        for (auto i : times)
        {
            fout << i.count() << std::endl;
        }
        fout.close();
    }

    std::sort(times.begin(), times.end());
    auto min = times.front();
    auto max = times.back();
    auto median = *(times.begin() + iterations / 2);
    
    double average = times.front().count();
    for (auto i : times)
        average += (double)i.count();
    average = average / iterations;

    cout << "End Test" << endl;
    cout << "Min: " << min.count() << endl;
    cout << "Max: " << max.count() << endl;
    cout << "Median: " << median.count() << endl;
    cout << "Average: " << average << endl;
    cout << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void ReadSamples(NIPerfTestClient* client, int numSamples, int numIterations)
{    
    int index = 0;
    grpc::ClientContext context;
    auto readResult = client->ReadContinuously(&context, 5.0, numSamples, numIterations);
    ReadContinuouslyResult cresult;
    while(readResult->Read(&cresult))
    {
        cresult.wfm().size();
        index += 1;
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void ReportMBPerSecond(chrono::high_resolution_clock::time_point start, chrono::high_resolution_clock::time_point end, int numSamples, int numIterations)
{
    int64_t elapsed = chrono::duration_cast<chrono::microseconds>(end - start).count();
    double elapsedSeconds = elapsed / (1000.0 * 1000.0);
    double bytesPerSecond = (8.0 * (double)numSamples * numIterations) / elapsedSeconds;
    double MBPerSecond = bytesPerSecond / (1024.0 * 1024);

    cout << numSamples << " Samples: " << MBPerSecond << " MB/s, " << elapsed << " total microseconds" << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void EnableTracing()
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/events/enable");
    // fout << "1";
    // fout.close();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void DisableTracing()
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/events/enable");
    // fout << "0";
    // fout.close();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void TracingOff()
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/tracing_on");
    // fout << "0";
    // fout.close();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void TracingOn()
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/tracing_on");
    // fout << "1";
    // fout.close();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void TraceMarker(const char* marker)
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/trace_marker");
    // fout << marker;
    // fout.close();
}
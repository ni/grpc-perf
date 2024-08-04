//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <client_utilities.h>
#include <sideband_data.h>
#include <sideband_grpc.h>
#include <performance_tests.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <perftest.grpc.pb.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <thread>
#include <src/core/lib/iomgr/executor.h>
#include <src/core/lib/iomgr/timer_manager.h>

#if (ENABLE_FLATBUFFERS)
#include <perftest_generated.h>
#include <perftest_lidar_generated.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
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
using namespace ni::data_monikers;
using namespace google::protobuf;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
static int LatencyTestIterations = 300000;
static int DefaultTestIterations = 20000;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#if (ENABLE_FLATBUFFERS)
// void PerformFlatbuffersPackLidarTableTest(int numSamples, int numIterations)
// {
//     cout << "Start flatbuffers pack lidar table test: " << numSamples << endl;

//     auto start = chrono::high_resolution_clock::now();
//     int packSize = 0;

//     for (int i=0; i<numIterations; ++i)
//     {
//         flatbuffers::FlatBufferBuilder builder(10 * 1024 * 1024);
//         std::vector<PerfTest::LidarTableValue> values;
//         values.reserve(numSamples);
//         for (int x=0; x<numSamples; ++x)
//         {
//             values.emplace_back(PerfTest::LidarTableValue(3.14, 4.56, 5.67, 6.78));
//         }
//         auto result = CreateReadLidarTableResultDirect(builder, &values, 1);
//         builder.Finish(result);
//         auto size = builder.GetSize();
//         packSize = size;
//     }

//     auto end = chrono::high_resolution_clock::now();
//     auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);

//     double timePerMessage = elapsed.count() / (numIterations);
//     cout << "Result: " << "PackSize: " << packSize << ", Microseconds per pack: " << timePerMessage << endl << endl;
// }
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#if (ENABLE_FLATBUFFERS)
void PerformFlatbuffersPackLidarTest(int numSamples, int numIterations)
{
    cout << "Start flatbuffers pack lidar test: " << numSamples << endl;

    auto start = chrono::high_resolution_clock::now();
    int packSize = 0;

    for (int i=0; i<numIterations; ++i)
    {
        flatbuffers::FlatBufferBuilder builder(10 * 1024 * 1024);
        std::vector<PerfTest::LidarValue> values;
        values.reserve(numSamples);
        for (int x=0; x<numSamples; ++x)
        {
            values.emplace_back(PerfTest::LidarValue(3.14, 4.56, 5.67, 6.78));
        }
        auto result = CreateReadLidarResultDirect(builder, &values, 1);
        builder.Finish(result);
        auto size = builder.GetSize();
        packSize = size;
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);

    double timePerMessage = (double)elapsed.count() / (double)numIterations;
    cout << "Result: " << "PackSize: " << packSize << ", Microseconds per pack: " << timePerMessage << endl << endl;
}
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#if (ENABLE_FLATBUFFERS)
void PerformFlatbuffersPackTest(int numSamples, int numIterations)
{
    cout << "Start flatbuffers pack test: " << numSamples << endl;

    auto start = chrono::high_resolution_clock::now();
    int packSize = 0;

    for (int i=0; i<numIterations; ++i)
    {
        flatbuffers::FlatBufferBuilder builder(10 * 1024 * 1024);
        std::vector<PerfTest::ComplexNumber> values;
        values.reserve(numSamples);
        for (int x=0; x<numSamples; ++x)
        {
            values.emplace_back(PerfTest::ComplexNumber(3.14, 4.56));
        }
        auto result = CreateReadComplexResultDirect(builder, &values, 1);
        builder.Finish(result);
        auto size = builder.GetSize();
        packSize = size;
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);

    double timePerMessage = (double)elapsed.count() / (double)numIterations;
    cout << "Result: " << "PackSize: " << packSize << ", Microseconds per pack: " << timePerMessage << endl << endl;
}
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#if (ENABLE_FLATBUFFERS)
void PerformFlatbuffersPackUnpackTest(int numSamples, int numIterations)
{
    cout << "Start flatbuffers pack unpack test: " << numSamples << endl;

    auto start = chrono::high_resolution_clock::now();
    int packSize = 0;

    for (int i=0; i<numIterations; ++i)
    {
        flatbuffers::FlatBufferBuilder builder(10 * 1024 * 1024);
        std::vector<PerfTest::ComplexNumber> values;
        values.reserve(numSamples);
        for (int x=0; x<numSamples; ++x)
        {
            values.emplace_back(PerfTest::ComplexNumber(3.14, 4.56));
        }
        auto result = CreateReadComplexResultDirect(builder, &values, 1);
        builder.Finish(result);
        auto size = builder.GetSize();
        packSize = size;

        std::vector<PerfTest::ComplexNumber> parsedValues;
        values.reserve(numSamples);
        auto parsedResult = PerfTest::GetReadComplexResult(builder.GetBufferPointer());
        for(auto it: *(parsedResult->values()))
        {
            parsedValues.emplace_back((*it).real(), (*it).imaginary());
        }
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);

    double timePerMessage = (double)elapsed.count() / (double)numIterations;
    cout << "Result: " << "PackSize: " << packSize << ", Microseconds per pack: " << timePerMessage << endl << endl;
}
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#if (ENABLE_FLATBUFFERS)
void PerformFlatbuffersPackUnpackLidarTest(int numSamples, int numIterations)
{
    cout << "Start flatbuffers pack unpack lidar test: " << numSamples << endl;

    auto start = chrono::high_resolution_clock::now();
    int packSize = 0;

    for (int i=0; i<numIterations; ++i)
    {
        flatbuffers::FlatBufferBuilder builder(10 * 1024 * 1024);
        std::vector<PerfTest::LidarValue> values;
        values.reserve(numSamples);
        for (int x=0; x<numSamples; ++x)
        {
            values.emplace_back(PerfTest::LidarValue(3.14, 4.56, 5.67, 6.78));
        }
        auto result = CreateReadLidarResultDirect(builder, &values, 1);
        builder.Finish(result);
        auto size = builder.GetSize();
        packSize = size;

        std::vector<PerfTest::LidarValue> parsedValues;
        parsedValues.reserve(numSamples);
        values.reserve(numSamples);
        auto parsedResult = PerfTest::GetReadLidarResult(builder.GetBufferPointer());
        for(auto it: *(parsedResult->values()))
        {
            // if (it->x() < 0)
            // {                
            //     PerfTest::LidarValue xx((*it).x(), (*it).y(), (*it).z(), (*it).a());
            //     parsedValues.emplace_back(xx);
            // }
        }
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);

    double timePerMessage = (double)elapsed.count() / (double)numIterations;
    cout << "Result: " << "PackSize: " << packSize << ", Microseconds per pack: " << timePerMessage << endl << endl;
}
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformArenaPackTest(int numSamples, int numIterations)
{
    cout << "Start arena pack test: " << numSamples << endl;

    std::string result;
    result.reserve(5000000);
    result.resize(5000000);

    int initial_block_size = 10 * 1024 * 1024;
    char* initial_block = new char [initial_block_size];

    int packSize = 0;    
    int spaceUsed = 0;
    {
        Arena arena(initial_block, initial_block_size);
        auto response = Arena::CreateMessage<niPerfTest::ReadComplexResult>(&arena);
        response->mutable_samples()->Reserve(numSamples);
        for (int x=0; x<numSamples; ++x)
        {
            auto sample = response->mutable_samples()->Add();
            sample->set_real(3.14);
            sample->set_imaginary(4.56);
        }
        response->set_status(0);

        auto start = chrono::high_resolution_clock::now();
        for (int i=0; i<numIterations; ++i)
        {
            for (int x=0; x<numSamples; ++x)
            {
                auto sample = response->mutable_samples()->Mutable(x);
                sample->set_real(3.14);
                sample->set_imaginary(4.56);
            }
            response->set_status(0);

            response->SerializeToString(&result);
            if (packSize == 0)
            {
                packSize = result.length();
                spaceUsed = arena.SpaceUsed();
            }
        }
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double timePerMessage = (double)elapsed.count() / (double)numIterations;
        cout << "Result: " << "Spaced Used: " << spaceUsed << ", Pack Size: " << packSize << ", Microseconds per pack: " << timePerMessage << endl << endl;
    }
    delete [] initial_block;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformArenaPackLidarTest(int numSamples, int numIterations)
{
    cout << "Start arena pack lidar test: " << numSamples << endl;

    std::string result;
    result.reserve(5000000);
    result.resize(5000000);

    int initial_block_size = 10 * 1024 * 1024;
    char* initial_block = new char [initial_block_size];

    int packSize = 0;    
    int spaceUsed = 0;
    {
        Arena arena(initial_block, initial_block_size);
        auto response = Arena::CreateMessage<niPerfTest::ReadLidarResult>(&arena);
        response->mutable_samples()->Reserve(numSamples);
        for (int x=0; x<numSamples; ++x)
        {
            auto sample = response->mutable_samples()->Add();
            sample->set_x(3.14);
            sample->set_y(4.56);
            sample->set_z(5.56);
            sample->set_a(6.56);
        }
        response->set_status(0);

        auto start = chrono::high_resolution_clock::now();
        for (int i=0; i<numIterations; ++i)
        {
            for (int x=0; x<numSamples; ++x)
            {
                auto sample = response->mutable_samples()->Mutable(x);
                sample->set_x(3.14);
                sample->set_y(4.56);
                sample->set_z(5.56);
                sample->set_a(6.56);
            }
            response->set_status(0);

            response->SerializeToString(&result);
            if (packSize == 0)
            {
                packSize = result.length();
                spaceUsed = arena.SpaceUsed();
            }
        }
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        double timePerMessage = (double)elapsed.count() / (double)numIterations;
        cout << "Result: " << "Spaced Used: " << spaceUsed << ", Pack Size: " << packSize << ", Microseconds per pack: " << timePerMessage << endl << endl;
    }
    delete [] initial_block;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformArenaPackUnpackTest(int numSamples, int numIterations)
{
    cout << "Start arena pack unpack test: " << numSamples << endl;

    std::string result;
    result.reserve(5000000);
    result.resize(5000000);

    int initial_block_size = 10 * 1024 * 1024;
    char* initial_block = new char [initial_block_size];

    int packSize = 0;    
    int spaceUsed = 0;
    {
        Arena arena(initial_block, initial_block_size);
        auto response = Arena::CreateMessage<niPerfTest::ReadComplexResult>(&arena);
        response->mutable_samples()->Reserve(numSamples);
        for (int x=0; x<numSamples; ++x)
        {
            auto sample = response->mutable_samples()->Add();
            sample->set_real(3.14);
            sample->set_imaginary(4.56);
        }
        response->set_status(0);

        auto parsed = Arena::CreateMessage<niPerfTest::ReadComplexResult>(&arena);
        parsed->mutable_samples()->Reserve(numSamples);
        for (int x=0; x<numSamples; ++x)
        {
            auto sample = parsed->mutable_samples()->Add();
            sample->set_real(3.14);
            sample->set_imaginary(4.56);
        }
        parsed->set_status(0);

        auto start = chrono::high_resolution_clock::now();
        for (int i=0; i<numIterations; ++i)
        {
            for (int x=0; x<numSamples; ++x)
            {
                auto sample = response->mutable_samples()->Mutable(x);
                sample->set_real(3.14);
                sample->set_imaginary(4.56);
            }
            response->set_status(0);

            response->SerializeToString(&result);
            if (packSize == 0)
            {
                packSize = result.length();
                spaceUsed = arena.SpaceUsed();
            }

            parsed->ParseFromString(result);
        }
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        double timePerMessage = (double)elapsed.count() / (double)numIterations;
        cout << "Result: " << "Spaced Used: " << spaceUsed << ", Pack Size: " << packSize << ", Microseconds per pack: " << timePerMessage << endl << endl;
    }
    delete [] initial_block;
}

struct MyData
{
    double x;
    double y;
    double z;
    double a;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformArenaPackUnpackLidarTest(int numSamples, int numIterations)
{
    cout << "Start arena pack unpack lidar test: " << numSamples << endl;

    std::string result;
    result.reserve(5000000);
    result.resize(5000000);

    std::vector<MyData> myResults;
    myResults.reserve(numSamples);
    myResults.resize(numSamples);

    int initial_block_size = 10 * 1024 * 1024;
    char* initial_block = new char [initial_block_size];

    int packSize = 0;    
    int spaceUsed = 0;
    {
        Arena arena(initial_block, initial_block_size);
        auto response = Arena::CreateMessage<niPerfTest::ReadLidarResult>(&arena);
        response->mutable_samples()->Reserve(numSamples);
        for (int x=0; x<numSamples; ++x)
        {
            auto sample = response->mutable_samples()->Add();
            sample->set_x(3.14);
            sample->set_y(4.56);
            sample->set_z(5.56);
            sample->set_a(6.56);
        }
        response->set_status(0);

        auto parsed = Arena::CreateMessage<niPerfTest::ReadLidarResult>(&arena);
        parsed->mutable_samples()->Reserve(numSamples);
        for (int x=0; x<numSamples; ++x)
        {
            auto sample = parsed->mutable_samples()->Add();
            sample->set_x(3.14);
            sample->set_y(4.56);
            sample->set_z(5.56);
            sample->set_a(6.56);
        }
        parsed->set_status(0);

        auto start = chrono::high_resolution_clock::now();
        for (int i=0; i<numIterations; ++i)
        {
            for (int x=0; x<numSamples; ++x)
            {
                auto sample = response->mutable_samples()->Mutable(x);
                sample->set_x(3.14);
                sample->set_y(4.56);
                sample->set_z(5.56);
                sample->set_a(6.56);
            }
            response->set_status(0);

            response->SerializeToString(&result);
            if (packSize == 0)
            {
                packSize = result.length();
                spaceUsed = arena.SpaceUsed();
            }

            parsed->ParseFromString(result);
            // int z = 0;
            // for (auto it: parsed->samples())
            // {
            //     if (it.x() < 0)
            //     {
            //         MyData r; //  = &(myResults.data()[z]);
            //         r.x = it.x();
            //         r.y = it.y();
            //         r.z = it.z();
            //         r.a = it.a();
            //         ++z;
            //     }
            // }
        }
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        double timePerMessage = (double)elapsed.count() / (double)numIterations;
        cout << "Result: " << "Spaced Used: " << spaceUsed << ", Pack Size: " << packSize << ", Microseconds per pack: " << timePerMessage << endl << endl;
    }
    delete [] initial_block;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformArenaPackVectorsTest(int numSamples, int numIterations)
{
    cout << "Start arena pack vectors test: " << numSamples << endl;

    std::string result;
    result.reserve(10 * 1024 * 1024);

    int initial_block_size = 10 * 1024 * 1024;
    char* initial_block = new char [initial_block_size];

    int packSize = 0;    
    int spaceUsed = 0;
    {
        Arena arena(initial_block, initial_block_size);
        auto response = Arena::CreateMessage<niPerfTest::ReadComplexResult2>(&arena);
        response->mutable_samples()->mutable_real_values()->Reserve(numSamples);
        response->mutable_samples()->mutable_real_values()->Resize(numSamples, 3.14);
        response->mutable_samples()->mutable_imaginary_values()->Reserve(numSamples);
        response->mutable_samples()->mutable_imaginary_values()->Resize(numSamples, 4.56);
        response->set_status(0);

    auto start = chrono::high_resolution_clock::now();
    for (int i=0; i<numIterations; ++i)
    {
        for (int x=0; x<numSamples; ++x)
        {
            response->mutable_samples()->mutable_real_values()->Set(x, 3.14);
            response->mutable_samples()->mutable_imaginary_values()->Set(x, 3.45);
        }
        response->set_status(0);

        response->SerializeToString(&result);
        if (packSize == 0)
        {
            packSize = result.length();
            spaceUsed = arena.SpaceUsed();
        }
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double timePerMessage = (double)elapsed.count() / (double)numIterations;
    cout << "Result: " << "Spaced Used: " << spaceUsed << ", Pack Size: " << packSize << ", Microseconds per pack: " << timePerMessage << endl << endl;
    }
    delete [] initial_block;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformArenaPackLidarVectorsTest(int numSamples, int numIterations)
{
    cout << "Start arena pack lidar vectors test: " << numSamples << endl;

    std::string result;
    result.reserve(5000000);

    int initial_block_size = 10 * 1024 * 1024;
    char* initial_block = new char [initial_block_size];

    int packSize = 0;    
    int spaceUsed = 0;
    {
        Arena arena(initial_block, initial_block_size);
        auto response = Arena::CreateMessage<niPerfTest::ReadLidarResult2>(&arena);
        response->mutable_samples()->mutable_x_values()->Reserve(numSamples);
        response->mutable_samples()->mutable_x_values()->Resize(numSamples, 3.14);
        response->mutable_samples()->mutable_y_values()->Reserve(numSamples);
        response->mutable_samples()->mutable_y_values()->Resize(numSamples, 4.56);
        response->mutable_samples()->mutable_z_values()->Reserve(numSamples);
        response->mutable_samples()->mutable_z_values()->Resize(numSamples, 4.56);
        response->mutable_samples()->mutable_a_values()->Reserve(numSamples);
        response->mutable_samples()->mutable_a_values()->Resize(numSamples, 4.56);
        response->set_status(0);

    auto start = chrono::high_resolution_clock::now();
    for (int i=0; i<numIterations; ++i)
    {
        for (int x=0; x<numSamples; ++x)
        {
            response->mutable_samples()->mutable_x_values()->Set(x, 3.14);
            response->mutable_samples()->mutable_y_values()->Set(x, 3.45);
            response->mutable_samples()->mutable_z_values()->Set(x, 3.45);
            response->mutable_samples()->mutable_a_values()->Set(x, 3.45);
        }
        response->set_status(0);

        response->SerializeToString(&result);
        if (packSize == 0)
        {
            packSize = result.length();
            spaceUsed = arena.SpaceUsed();
        }
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double timePerMessage = (double)elapsed.count() / (double)numIterations;
    cout << "Result: " << "Spaced Used: " << spaceUsed << ", Pack Size: " << packSize << ", Microseconds per pack: " << timePerMessage << endl << endl;
    }
    delete [] initial_block;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformArenaPackUnpackVectorsTest(int numSamples, int numIterations)
{
    cout << "Start arena pack unpack vectors test: " << numSamples << endl;

    std::string result;
    result.reserve(5000000);

    int initial_block_size = 10 * 1024 * 1024;
    char* initial_block = new char [initial_block_size];

    int packSize = 0;    
    int spaceUsed = 0;
    {
        Arena arena(initial_block, initial_block_size);
        auto response = Arena::CreateMessage<niPerfTest::ReadComplexResult2>(&arena);
        response->mutable_samples()->mutable_real_values()->Reserve(numSamples);
        response->mutable_samples()->mutable_real_values()->Resize(numSamples, 3.14);
        response->mutable_samples()->mutable_imaginary_values()->Reserve(numSamples);
        response->mutable_samples()->mutable_imaginary_values()->Resize(numSamples, 4.56);
        response->set_status(0);

        auto parsed = Arena::CreateMessage<niPerfTest::ReadComplexResult2>(&arena);

        auto start = chrono::high_resolution_clock::now();
        for (int i=0; i<numIterations; ++i)
        {
            for (int x=0; x<numSamples; ++x)
            {
                response->mutable_samples()->mutable_real_values()->Set(x, 3.14);
                response->mutable_samples()->mutable_imaginary_values()->Set(x, 3.45);
            }
            response->set_status(0);

            response->SerializeToString(&result);
            if (packSize == 0)
            {
                packSize = result.length();
                spaceUsed = arena.SpaceUsed();
            }
            //auto parsed = Arena::CreateMessage<niPerfTest::ReadComplexResult>(&arena);
            parsed->ParseFromString(result);
        }

        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        double timePerMessage = (double)elapsed.count() / (double)numIterations;
        cout << "Result: " << "Spaced Used: " << spaceUsed << ", Pack Size: " << packSize << ", Microseconds per pack: " << timePerMessage << endl << endl;
    }
    delete [] initial_block;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformArenaPackUnpackLidarVectorsTest(int numSamples, int numIterations)
{
    cout << "Start arena pack unpack lidar vectors test: " << numSamples << endl;

    std::string result;
    result.reserve(10 * 1024 * 1024);

    int initial_block_size = 10 * 1024 * 1024;
    char* initial_block = new char [initial_block_size];

    int packSize = 0;    
    int spaceUsed = 0;
    {
        Arena arena(initial_block, initial_block_size);
        auto response = Arena::CreateMessage<niPerfTest::ReadLidarResult2>(&arena);
        response->mutable_samples()->mutable_x_values()->Reserve(numSamples);
        response->mutable_samples()->mutable_x_values()->Resize(numSamples, 3.14);
        response->mutable_samples()->mutable_y_values()->Reserve(numSamples);
        response->mutable_samples()->mutable_y_values()->Resize(numSamples, 4.56);
        response->mutable_samples()->mutable_z_values()->Reserve(numSamples);
        response->mutable_samples()->mutable_z_values()->Resize(numSamples, 4.56);
        response->mutable_samples()->mutable_a_values()->Reserve(numSamples);
        response->mutable_samples()->mutable_a_values()->Resize(numSamples, 4.56);
        response->set_status(0);

        auto parsed = Arena::CreateMessage<niPerfTest::ReadLidarResult2>(&arena);

        auto start = chrono::high_resolution_clock::now();
        for (int i=0; i<numIterations; ++i)
        {
            for (int x=0; x<numSamples; ++x)
            {
                response->mutable_samples()->mutable_x_values()->Set(x, 3.14);
                response->mutable_samples()->mutable_y_values()->Set(x, 3.45);
                response->mutable_samples()->mutable_z_values()->Set(x, 3.45);
                response->mutable_samples()->mutable_a_values()->Set(x, 3.45);
            }
            response->set_status(0);

            response->SerializeToString(&result);
            if (packSize == 0)
            {
                packSize = result.length();
                spaceUsed = arena.SpaceUsed();
            }
            //auto parsed = Arena::CreateMessage<niPerfTest::ReadComplexResult>(&arena);
            parsed->ParseFromString(result);
        }

        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        double timePerMessage = (double)elapsed.count() / (double)numIterations;
        cout << "Result: " << "Spaced Used: " << spaceUsed << ", Pack Size: " << packSize << ", Microseconds per pack: " << timePerMessage << endl << endl;
    }
    delete [] initial_block;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformPackUnpackTest(int numSamples, int numIterations)
{
    cout << "Start pack unpack test: " << numIterations << endl;

    auto start = chrono::high_resolution_clock::now();
    std::string result;
    result.reserve(5000000);

    auto packSize = 0;
    for (int i=0; i<numIterations; ++i)
    {
        auto response = new niPerfTest::ReadComplexResult();
        response->mutable_samples()->Reserve(numSamples);
        for (int x=0; x<numSamples; ++x)
        {
            auto sample = response->mutable_samples()->Add();
            sample->set_real(3.14);
            sample->set_imaginary(4.56);
        }
        response->set_status(0);

        response->SerializeToString(&result);
        delete response;
        packSize = result.length();
    }
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);

    double timePerMessage = (double)elapsed.count() / (double)numIterations;
    cout << "Result: " << "PackSize: " << packSize << ", Microseconds per pack: " << timePerMessage << endl << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformAsyncInitTest(NIPerfTestClient& client, int numCommands, int numIterations)
{
    cout << "Start async init test, num commands: " << numCommands << endl;

    auto start = chrono::high_resolution_clock::now();
    for (int i=0; i<numIterations; ++i)
    {
        AsyncInitResults* results = new AsyncInitResults[numCommands];
        grpc::CompletionQueue cq;
        for (int x=0; x<numCommands; ++x)
        {
            client.InitAsync(42+x, "This is a command", cq, &results[x]);
        }
        std::vector<AsyncInitResults*> completedList;
        while (completedList.size() != numCommands)
        {
            AsyncInitResults* completed;
            bool ok;
            cq.Next((void**)&completed, &ok);
            completedList.push_back(completed);
            if (!ok)
            {
                cout << "Completion Queue returned error!" << endl;
                return;
            }
            if (completed->reply.status() < 42 || completed->reply.status() > (42+numCommands))
            {
                cout << "BAD REPLY!" << endl;
            }
            if (!completed->status.ok())
            {
                cout << completed->status.error_code() << ": " << completed->status.error_message() << endl;
            }
        }
        delete[] results;
    }
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);

    double msgsPerSecond = (numCommands * numIterations * 1000.0 * 1000.0) / (double)elapsed.count();
    double timePerMessage = elapsed.count() / (numCommands * numIterations);
    cout << "Result: " << msgsPerSecond << " reads per second, Microseconds per read: " << timePerMessage << endl << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformScopeLikeRead(NIPerfTestClient& client)
{
    int samples = 10;
    double* buffer = new double[samples];

    cout << "Start Scope Like Read test" << endl;
    auto start = chrono::high_resolution_clock::now();
    for (int x=0; x<10000; ++x)
    {
        client.ConfigureVertical("", "0",10.0, 0.0, VERTICAL_COUPLING_NISCOPE_VAL_AC, 1.0, true);
        client.ConfigureHorizontalTiming("", 50000000, samples, 50.0, 1, true);
        client.InitiateAcquisition("");
        client.Read(1000, samples, buffer);
    }
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double timePerTest = (double)elapsed.count() / 10000.0;
    delete [] buffer;

    cout << "Result: " << timePerTest << " us Per iteration" << endl << endl;
}

#if (ENABLE_GRPC_SIDEBAND)
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void ThreadPerformSidebandMonikerLatencyTest(MonikerClient* client, int numSamples, ni::data_monikers::SidebandStrategy strategy)
{
    std::cout << "Start Sideband moniker latency test, "  << numSamples << " Samples" << std::endl;
    ClientContext ctx;
    BeginMonikerSidebandStreamRequest request;
    request.set_strategy(strategy);
    BeginMonikerSidebandStreamResponse response;
    client->m_Stub->BeginSidebandStream(&ctx, request, &response);

#ifndef _WIN32
    if (strategy == ni::data_monikers::SidebandStrategy::RDMA_LOW_LATENCY ||
        strategy == ni::data_monikers::SidebandStrategy::SOCKETS_LOW_LATENCY)
    {
        cpu_set_t cpuSet;
        CPU_ZERO(&cpuSet);
        CPU_SET(10, &cpuSet);
        CPU_SET(11, &cpuSet);
        pid_t threadId = syscall(SYS_gettid);
        auto result = sched_setaffinity(threadId, sizeof(cpu_set_t), &cpuSet);
    }
#endif

    timeVector times;
    times.reserve(10000);

    auto sidebandToken = InitClientSidebandData(response);

    ReadResult toPack;
    toPack.set_status(42);
    for (int x=0; x<numSamples; ++x)
    {
        toPack.add_samples(x);
    }

    SidebandWriteRequest sidebandRequest;
    auto value = sidebandRequest.mutable_values()->add_values();
    value->PackFrom(toPack);

    MonikerReadResult sidebandResponse;
    for (int x=0; x<10; ++x)
    {
        WriteSidebandMessage(sidebandToken, sidebandRequest);
        ReadSidebandMessage(sidebandToken, &sidebandResponse);
    }

    for (int x=0; x<100; ++x)
    {
        auto start = chrono::high_resolution_clock::now();
        WriteSidebandMessage(sidebandToken, sidebandRequest);
        ReadSidebandMessage(sidebandToken, &sidebandResponse);
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
        std::this_thread::sleep_for(std::chrono::milliseconds(0));
    }
    sidebandRequest.set_cancel(true);
    WriteSidebandMessage(sidebandToken, sidebandRequest);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    CloseSidebandData(sidebandToken);
    WriteLatencyData(times, "SidebandLatency.txt");
    std::cout << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformSidebandMonikerLatencyTest(MonikerClient& client, int numSamples, ni::data_monikers::SidebandStrategy strategy)
{
    auto thread = new std::thread(ThreadPerformSidebandMonikerLatencyTest, &client, numSamples, strategy);
    thread->join();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformSidebandReadTest(NIPerfTestClient& client, int numSamples, ni::data_monikers::SidebandStrategy strategy, const std::string& message)
{
    cout << "Start Sideband Read Test " << message << endl;

    uint8_t* buffer = new uint8_t[numSamples];
    BeginTestSidebandStreamResponse response;
    {
        ClientContext context;
        BeginTestSidebandStreamRequest request;
        request.set_strategy(strategy);
        request.set_num_samples(numSamples);
        client.m_Stub->BeginTestSidebandStream(&context, request, &response);
    }
    auto sidebandIdentifier = response.sideband_identifier();
    int64_t sidebandToken = 0;
    InitClientSidebandData(response.connection_url().c_str(), (::SidebandStrategy)response.strategy(), response.sideband_identifier().c_str(), numSamples, &sidebandToken);
    {
        ClientContext context;
        auto stream = client.m_Stub->TestSidebandStream(&context);

        TestSidebandStreamRequest readParameters;
        readParameters.set_strategy(strategy);
        readParameters.set_sideband_identifier(sidebandIdentifier);
        readParameters.set_num_samples(numSamples);

        auto start = chrono::high_resolution_clock::now();
        for (int x = 0; x < DefaultTestIterations; ++x)
        {
            TestSidebandStreamResponse response;
            stream->Write(readParameters);
            stream->Read(&response);
            int64_t bytesRead;
            ReadSidebandData(sidebandToken, buffer, numSamples, &bytesRead);
        }
        auto end = chrono::high_resolution_clock::now();
        ReportMBPerSecond(start, end, numSamples / 8, DefaultTestIterations);
        CloseSidebandData(sidebandToken);
        stream->WritesDone();
        stream->Finish();
    }
}
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformMessagePerformanceTest(NIPerfTestClient& client)
{
    cout << "Start Messages per second test" << endl;

    auto start = chrono::high_resolution_clock::now();
    for (int x=0; x<50000; ++x)
    {
        client.Init(42);
    }
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double msgsPerSecond = (50000.0 * 1000.0 * 1000.0) / (double)elapsed.count();

    cout << "Result: " << msgsPerSecond << " messages/s" << endl << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformLatencyStreamTest(NIPerfTestClient& client, std::string fileName)
{    
    cout << "Start RPC Stream latency test, iterations=" << LatencyTestIterations << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    timeVector times;
    times.reserve(LatencyTestIterations);

	niPerfTest::StreamLatencyClient clientData;
	niPerfTest::StreamLatencyServer serverData;

    ClientContext context;
    auto stream = client.m_Stub->StreamLatencyTest(&context);
    for (int x=0; x<10; ++x)
    {
        stream->Write(clientData);
        stream->Read(&serverData);
    }

    EnableTracing();
    for (int x=0; x<LatencyTestIterations; ++x)
    {
        TraceMarker("Start iteration");
        //clientData.set_message(x);
        auto start = chrono::high_resolution_clock::now();
        stream->Write(clientData);
        stream->Read(&serverData);
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
        // if (elapsed.count() > 200)
        // {
        //     TraceMarker("High Latency");
        //     DisableTracing();
        //     cout << "HIGH Latency: " << x << "iterations" << endl;
        //     break;
        // }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    WriteLatencyData(times, fileName);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

std::unique_ptr<grpc::ByteBuffer> SerializeToByteBuffer(
    const grpc::protobuf::Message& message)
{
    std::string buf;
    message.SerializeToString(&buf);
    grpc::Slice slice(buf);
    return std::unique_ptr<grpc::ByteBuffer>(new grpc::ByteBuffer(&slice, 1));
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformLatencyPayloadWriteTest(NIPerfTestClient& client, int numSamples, std::string fileName)
{    
    cout << "Start RPC Latency payload write test, iterations=" << LatencyTestIterations << " numSamples=" << numSamples << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    timeVector times;
    times.reserve(LatencyTestIterations);

    TestWriteParameters request;
    request.mutable_samples()->Reserve(numSamples);
    request.mutable_samples()->Resize(numSamples, 0);
    TestWriteResult reply;

    for (int x=0; x<100; ++x)
    {
        ClientContext context;
        client.m_Stub->TestWrite(&context, request, &reply);
    }
    EnableTracing();
    for (int x=0; x<LatencyTestIterations; ++x)
    {
        TraceMarker("Start iteration");
        auto start = chrono::high_resolution_clock::now();
        ClientContext context;
        client.m_Stub->TestWrite(&context, request, &reply);
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
    }
    WriteLatencyData(times, fileName);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformLatencyPayloadWriteStreamTest(NIPerfTestClient& client, int numSamples, std::string fileName)
{    
    cout << "Start RPC Latency payload write stream test, iterations=" << LatencyTestIterations << " numSamples=" << numSamples << endl;

    timeVector times;
    times.reserve(LatencyTestIterations);

    TestWriteParameters request;
    request.mutable_samples()->Reserve(numSamples);
    request.mutable_samples()->Resize(numSamples, 0);
    TestWriteResult reply;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    ClientContext context;
    auto stream = client.m_Stub->TestWriteContinuously(&context);
    for (int x=0; x<100; ++x)
    {
        stream->Write(request);
        stream->Read(&reply);
    }
    EnableTracing();
    for (int x=0; x<LatencyTestIterations; ++x)
    {
        TraceMarker("Start iteration");
        auto start = chrono::high_resolution_clock::now();
        stream->Write(request);
        stream->Read(&reply);
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
    }
    WriteLatencyData(times, fileName);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
struct StreamInfo
{
    ClientContext context;
    niPerfTest::StreamLatencyClient clientData;
    std::unique_ptr< ::grpc::ClientReader< ::niPerfTest::StreamLatencyServer>> rstream;

    ClientContext wcontext;
    std::unique_ptr< ::grpc::ClientWriter< ::niPerfTest::StreamLatencyClient>> wstream;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformLatencyStreamTest2(NIPerfTestClient& client, NIPerfTestClient& client2, int streamCount, std::string fileName)
{    
    cout << "Start RPC Stream latency test, streams:" << streamCount << ", iterations=" << LatencyTestIterations << endl;

    timeVector times;
    times.reserve(LatencyTestIterations);

    StreamInfo* streamInfos = new StreamInfo[streamCount];
	niPerfTest::StreamLatencyServer serverData;
	niPerfTest::StreamLatencyServer serverResponseData;

    for (int x=0; x<streamCount; ++x)
    {
        streamInfos[x].clientData.set_message(x);       
        streamInfos[x].rstream = client.m_Stub->StreamLatencyTestServer(&streamInfos[x].context, streamInfos[x].clientData);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        streamInfos[x].wstream = client2.m_Stub->StreamLatencyTestClient(&streamInfos[x].wcontext, &serverResponseData);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    for (int x=0; x<100; ++x)
    {
        for (int i=0; i<streamCount; ++i)
        {
            streamInfos[i].wstream->Write(streamInfos[i].clientData);
        }
        for (int i=0; i<streamCount; ++i)
        {
            streamInfos[i].rstream->Read(&serverData);
        }
    }
    for (int x=0; x<LatencyTestIterations; ++x)
    {
        auto start = chrono::high_resolution_clock::now();
        for (int i=0; i<streamCount; ++i)
        {
            streamInfos[i].wstream->Write(streamInfos[i].clientData);
        }
        for (int i=0; i<streamCount; ++i)
        {
            streamInfos[i].rstream->Read(&serverData);
        }
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
    }

    for (int x=0; x<streamCount; ++x)
    {
        streamInfos[x].wstream->WritesDone();
    }

    delete [] streamInfos;
    WriteLatencyData(times, fileName);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformMessageLatencyTest(NIPerfTestClient& client, std::string fileName)
{
    cout << "Start RPC latency test, iterations=" << LatencyTestIterations << endl;

    timeVector times;
    times.reserve(LatencyTestIterations);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    InitParameters request;
    request.set_id(123);

    InitResult reply;

    for (int x=0; x<100; ++x)
    {
        ClientContext context;
        client.m_Stub->Init(&context, request, &reply);
    }
    for (int x=0; x<LatencyTestIterations; ++x)
    {
        auto start = chrono::high_resolution_clock::now();
        ClientContext context;
        client.m_Stub->Init(&context, request, &reply);
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
    }
    WriteLatencyData(times, fileName);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformReadTest(NIPerfTestClient& client, int numSamples, int numIterations)
{    
    cout << "Start " << numSamples << " Read Test" << endl;

    int index = 0;
    double* samples = new double[numSamples];

    auto start = chrono::high_resolution_clock::now();
    for (int x=0; x<numIterations; ++x)
    {
        client.Read(1000, numSamples, samples);
    }
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double msgsPerSecond = (numIterations * 1000.0 * 1000.0) / (double)elapsed.count();
    double timePerMessage = elapsed.count() / numIterations;

    delete [] samples;
    cout << "Result: " << msgsPerSecond << " reads per second, Microseconds per read: " << timePerMessage << endl;
    ReportMBPerSecond(start, end, numSamples, numIterations);
    std::cout << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformReadComplexTest(NIPerfTestClient& client, int numSamples, int numIterations)
{    
    cout << "Start " << numSamples << " Read Complex Test" << endl;

    int index = 0;

    auto start = chrono::high_resolution_clock::now();
    for (int x=0; x<numIterations; ++x)
    {
        client.ReadComplex(1000, numSamples);
    }
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double msgsPerSecond = (numIterations * 1000.0 * 1000.0) / (double)elapsed.count();
    double timePerMessage = elapsed.count() / numIterations;

    cout << "Result: " << msgsPerSecond << " reads per second, Microseconds per read: " << timePerMessage << endl;
    ReportMBPerSecond(start, end, numSamples, numIterations);
    std::cout << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformReadComplexArenaTest(NIPerfTestClient& client, int numSamples, int numIterations)
{    
    cout << "Start " << numSamples << " Read Complex Arena Test" << endl;

    int index = 0;

    auto start = chrono::high_resolution_clock::now();
    for (int x=0; x<numIterations; ++x)
    {
        client.ReadComplexArena(1000, numSamples);
    }
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double msgsPerSecond = (numIterations * 1000.0 * 1000.0) / (double)elapsed.count();
    double timePerMessage = elapsed.count() / numIterations;

    cout << "Result: " << msgsPerSecond << " reads per second, Microseconds per read: " << timePerMessage << endl;
    ReportMBPerSecond(start, end, numSamples, numIterations);
    std::cout << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformWriteTest(NIPerfTestClient& client, int numSamples)
{   
    cout << "Start " << numSamples << " Write Test" << endl;

    int index = 0;
    double* samples = new double[numSamples];

    auto start = chrono::high_resolution_clock::now();
    for (int x=0; x<1000; ++x)
    {
        client.TestWrite(numSamples, samples);
        if (x == 0 || x == 2)
        {
            cout << ",";
        }
        if ((x % 20) == 0)
        {
            cout << ".";
        }
    }
    cout << endl;
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double msgsPerSecond = (1000.0 * 1000.0 * 1000.0) / (double)elapsed.count();

    delete [] samples;
    cout << "Result: " << msgsPerSecond << " reads per second" << endl << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformStreamingTest(NIPerfTestClient& client, int numSamples)
{
    auto start = chrono::high_resolution_clock::now();
    ReadSamples(&client, numSamples, DefaultTestIterations);
    auto end = chrono::high_resolution_clock::now();
    ReportMBPerSecond(start, end, numSamples, DefaultTestIterations);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformTwoStreamTest(NIPerfTestClient& client, NIPerfTestClient& client2, int numSamples)
{
    auto start = chrono::high_resolution_clock::now();

    auto thread1 = new thread(ReadSamples, &client, numSamples, DefaultTestIterations);
    auto thread2 = new thread(ReadSamples, &client2, numSamples, DefaultTestIterations);

    thread1->join();
    thread2->join();

    auto end = chrono::high_resolution_clock::now();
    ReportMBPerSecond(start, end, numSamples * 2, DefaultTestIterations);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformFourStreamTest(NIPerfTestClient& client, NIPerfTestClient& client2, NIPerfTestClient& client3, NIPerfTestClient& client4, int numSamples)
{
    auto start = chrono::high_resolution_clock::now();

    auto thread1 = new thread(ReadSamples, &client, numSamples, DefaultTestIterations);
    auto thread2 = new thread(ReadSamples, &client2, numSamples, DefaultTestIterations);
    auto thread3 = new thread(ReadSamples, &client3, numSamples, DefaultTestIterations);
    auto thread4 = new thread(ReadSamples, &client4, numSamples, DefaultTestIterations);

    thread1->join();
    thread2->join();
    thread3->join();
    thread4->join();

    auto end = chrono::high_resolution_clock::now();
    ReportMBPerSecond(start, end, numSamples * 4, DefaultTestIterations);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformNStreamTest(std::vector<NIPerfTestClient*>& clients, int numSamples)
{
    auto start = chrono::high_resolution_clock::now();

    std::vector<thread*> threads;
    threads.reserve(clients.size());
    for (auto client: clients)
    {
        auto t = new thread(ReadSamples, client, numSamples, DefaultTestIterations);
        threads.emplace_back(t);
    }
    for (auto thread: threads)
    {
        thread->join();
    }
    auto end = chrono::high_resolution_clock::now();
    ReportMBPerSecond(start, end, numSamples * clients.size(), DefaultTestIterations);
}

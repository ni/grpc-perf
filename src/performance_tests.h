//---------------------------------------------------------------------
//---------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class NIPerfTestClient;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformMessagePerformanceTest(NIPerfTestClient& client);
void PerformLatencyStreamTest(NIPerfTestClient& client, std::string fileName);
void PerformLatencyPayloadWriteTest(NIPerfTestClient& client, int numSamples, std::string fileName);
void PerformLatencyPayloadWriteStreamTest(NIPerfTestClient& client, int numSamples, std::string fileName);
void PerformLatencyStreamTest2(NIPerfTestClient& client, NIPerfTestClient& client2, int streamCount, std::string fileName);
void PerformMessageLatencyTest(NIPerfTestClient& client, std::string fileName);
void PerformReadTest(NIPerfTestClient& client, int numSamples, int numIterations);
void PerformReadComplexTest(NIPerfTestClient& client, int numSamples, int numIterations);
void PerformReadComplexArenaTest(NIPerfTestClient& client, int numSamples, int numIterations);
void PerformWriteTest(NIPerfTestClient& client, int numSamples);
void PerformStreamingTest(NIPerfTestClient& client, int numSamples);
void PerformTwoStreamTest(NIPerfTestClient& client, NIPerfTestClient& client2, int numSamples);
void PerformFourStreamTest(NIPerfTestClient& client, NIPerfTestClient& client2, NIPerfTestClient& client3, NIPerfTestClient& client4, int numSamples);
void PerformNStreamTest(std::vector<NIPerfTestClient*>& clients, int numSamples);
void PerformAsyncInitTest(NIPerfTestClient& client, int numCommands, int numIterations);
void PerformScopeLikeRead(NIPerfTestClient& client);

void PerformArenaPackTest(int numSamples, int numIterations);
void PerformArenaPackLidarTest(int numSamples, int numIterations);
void PerformArenaPackUnpackTest(int numSamples, int numIterations);
void PerformArenaPackUnpackLidarTest(int numSamples, int numIterations);
void PerformArenaPackVectorsTest(int numSamples, int numIterations);
void PerformArenaPackLidarVectorsTest(int numSamples, int numIterations);
void PerformArenaPackUnpackVectorsTest(int numSamples, int numIterations);
void PerformArenaPackUnpackLidarVectorsTest(int numSamples, int numIterations);
void PerformPackUnpackTest(int numSamples, int numIterations);

#if (ENABLE_FLATBUFFERS)
void PerformFlatbuffersPackTest(int numSamples, int numIterations);
void PerformFlatbuffersPackUnpackTest(int numSamples, int numIterations);
void PerformFlatbuffersPackLidarTest(int numSamples, int numIterations);
void PerformFlatbuffersPackLidarTableTest(int numSamples, int numIterations);
void PerformFlatbuffersPackUnpackLidarTest(int numSamples, int numIterations);
#endif

#if ENABLE_GRPC_SIDEBAND
void PerformSidebandReadTest(NIPerfTestClient& client, int numSamples, ni::data_monikers::SidebandStrategy strategy, const std::string& message);
void PerformSidebandMonikerLatencyTest(MonikerClient& client, int numSamples, ni::data_monikers::SidebandStrategy strategy);
#endif
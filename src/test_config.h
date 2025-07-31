//---------------------------------------------------------------------
//---------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include <memory>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
struct TestSuiteConfig
{
    std::string name;
    bool enabled;
    std::vector<std::string> parameters;
};

struct AsyncInitTestConfig
{
    bool enabled;
    std::vector<int> command_counts;
    int iterations;
};

struct ParallelStreamTestConfig
{
    bool enabled;
    std::vector<int> client_counts;
    std::vector<int> sample_counts;
};

struct PayloadTestConfig
{
    bool enabled;
    std::vector<int> payload_sizes;
    std::string output_prefix;
};

struct SidebandTestConfig
{
    bool enabled;
    std::vector<int> buffer_sizes;
    std::vector<std::string> strategies;
};

struct PackingTestConfig
{
    bool enabled;
    std::vector<int> sample_counts;
    std::vector<int> iteration_counts;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class TestConfig
{
public:
    // Test suite configurations
    TestSuiteConfig message_performance;
    TestSuiteConfig latency_stream;
    TestSuiteConfig message_latency;
    TestSuiteConfig read_tests;
    TestSuiteConfig read_complex_tests;
    TestSuiteConfig write_tests;
    TestSuiteConfig streaming_tests;
    TestSuiteConfig scpi_compare_tests;
    TestSuiteConfig uds_tests;
    
    // Specialized test configurations
    AsyncInitTestConfig async_init_tests;
    ParallelStreamTestConfig parallel_stream_tests;
    PayloadTestConfig payload_write_tests;
    PayloadTestConfig payload_stream_tests;
    SidebandTestConfig sideband_tests;
    PackingTestConfig packing_tests;

    // Individual test configurations
    struct {
        bool enabled = false;
        std::vector<int> payload_sizes;
        std::string filename;
    } single_latency_stream;

    // Load configuration from JSON file
    bool LoadFromFile(const std::string& filename);
    
    // Save default configuration to JSON file
    bool SaveDefaultToFile(const std::string& filename);

private:
    void SetDefaults();
};

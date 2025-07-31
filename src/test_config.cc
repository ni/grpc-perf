//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include "test_config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;
using namespace std;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool TestConfig::LoadFromFile(const string& filename)
{
    try
    {
        ifstream file(filename);
        if (!file.is_open())
        {
            cout << "Warning: Could not open config file '" << filename << "'. Using defaults." << endl;
            SetDefaults();
            return false;
        }

        json config_json;
        file >> config_json;

        // Load test suite configurations
        if (config_json.contains("test_suites"))
        {
            auto& suites = config_json["test_suites"];
            
            message_performance.enabled = suites.value("message_performance", false);
            latency_stream.enabled = suites.value("latency_stream", false);
            message_latency.enabled = suites.value("message_latency", false);
            read_tests.enabled = suites.value("read_tests", false);
            read_complex_tests.enabled = suites.value("read_complex_tests", false);
            write_tests.enabled = suites.value("write_tests", false);
            streaming_tests.enabled = suites.value("streaming_tests", false);
            scpi_compare_tests.enabled = suites.value("scpi_compare_tests", false);
            uds_tests.enabled = suites.value("uds_tests", false);
        }

        // Load async init test configuration
        if (config_json.contains("async_init_tests"))
        {
            auto& async_config = config_json["async_init_tests"];
            async_init_tests.enabled = async_config.value("enabled", false);
            async_init_tests.command_counts = async_config.value("command_counts", vector<int>{2, 3, 5, 10});
            async_init_tests.iterations = async_config.value("iterations", 10000);
        }

        // Load parallel stream test configuration
        if (config_json.contains("parallel_stream_tests"))
        {
            auto& parallel_config = config_json["parallel_stream_tests"];
            parallel_stream_tests.enabled = parallel_config.value("enabled", false);
            parallel_stream_tests.client_counts = parallel_config.value("client_counts", vector<int>{2, 4, 8, 16});
            parallel_stream_tests.sample_counts = parallel_config.value("sample_counts", vector<int>{1000, 10000, 100000, 200000});
        }

        // Load payload test configurations
        if (config_json.contains("payload_write_tests"))
        {
            auto& payload_config = config_json["payload_write_tests"];
            payload_write_tests.enabled = payload_config.value("enabled", false);
            payload_write_tests.payload_sizes = payload_config.value("payload_sizes", vector<int>{1, 8, 16, 32, 64, 128, 1024, 32768});
            payload_write_tests.output_prefix = payload_config.value("output_prefix", string("payloadlatency"));
        }

        if (config_json.contains("payload_stream_tests"))
        {
            auto& payload_stream_config = config_json["payload_stream_tests"];
            payload_stream_tests.enabled = payload_stream_config.value("enabled", false);
            payload_stream_tests.payload_sizes = payload_stream_config.value("payload_sizes", vector<int>{1, 8, 16, 32, 64, 128, 1024, 32768});
            payload_stream_tests.output_prefix = payload_stream_config.value("output_prefix", string("payloadstreamlatency"));
        }

        // Load sideband test configuration
        if (config_json.contains("sideband_tests"))
        {
            auto& sideband_config = config_json["sideband_tests"];
            sideband_tests.enabled = sideband_config.value("enabled", false);
            sideband_tests.buffer_sizes = sideband_config.value("buffer_sizes", vector<int>{4 * 1024 * 1024});
            sideband_tests.strategies = sideband_config.value("strategies", vector<string>{"SOCKETS", "RDMA"});
        }

        // Load packing test configuration
        if (config_json.contains("packing_tests"))
        {
            auto& packing_config = config_json["packing_tests"];
            packing_tests.enabled = packing_config.value("enabled", false);
            packing_tests.sample_counts = packing_config.value("sample_counts", vector<int>{1000, 10000, 100000});
            packing_tests.iteration_counts = packing_config.value("iteration_counts", vector<int>{5000, 10000});
        }

        // Load single latency stream test
        if (config_json.contains("single_latency_stream"))
        {
            auto& single_config = config_json["single_latency_stream"];
            single_latency_stream.enabled = single_config.value("enabled", false);
            single_latency_stream.filename = single_config.value("filename", string("streamlatency1.txt"));
        }

        return true;
    }
    catch (const exception& e)
    {
        cout << "Error parsing config file: " << e.what() << endl;
        SetDefaults();
        return false;
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool TestConfig::SaveDefaultToFile(const string& filename)
{
    try
    {
        json config_json;

        // Test suites
        config_json["test_suites"] = {
            {"message_performance", true},
            {"latency_stream", false},
            {"message_latency", false},
            {"read_tests", false},
            {"read_complex_tests", false},
            {"write_tests", false},
            {"streaming_tests", false},
            {"scpi_compare_tests", false},
            {"uds_tests", false}
        };

        // Async init tests
        config_json["async_init_tests"] = {
            {"enabled", false},
            {"command_counts", vector<int>{2, 3, 5, 10}},
            {"iterations", 10000}
        };

        // Parallel stream tests
        config_json["parallel_stream_tests"] = {
            {"enabled", false},
            {"client_counts", vector<int>{2, 4, 8, 16}},
            {"sample_counts", vector<int>{1000, 10000, 100000, 200000}}
        };

        // Payload tests
        config_json["payload_write_tests"] = {
            {"enabled", false},
            {"payload_sizes", vector<int>{1, 8, 16, 32, 64, 128, 1024, 32768}},
            {"output_prefix", "payloadlatency"}
        };

        config_json["payload_stream_tests"] = {
            {"enabled", false},
            {"payload_sizes", vector<int>{1, 8, 16, 32, 64, 128, 1024, 32768}},
            {"output_prefix", "payloadstreamlatency"}
        };

        // Sideband tests
        config_json["sideband_tests"] = {
            {"enabled", false},
            {"buffer_sizes", vector<int>{4 * 1024 * 1024}},
            {"strategies", vector<string>{"SOCKETS", "RDMA"}}
        };

        // Packing tests
        config_json["packing_tests"] = {
            {"enabled", false},
            {"sample_counts", vector<int>{1000, 10000, 100000}},
            {"iteration_counts", vector<int>{5000, 10000}}
        };

        // Single latency stream test
        config_json["single_latency_stream"] = {
            {"enabled", true},
            {"filename", "streamlatency1.txt"}
        };

        ofstream file(filename);
        file << config_json.dump(4);
        return true;
    }
    catch (const exception& e)
    {
        cout << "Error saving config file: " << e.what() << endl;
        return false;
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void TestConfig::SetDefaults()
{
    // Set reasonable defaults
    message_performance.enabled = true;
    latency_stream.enabled = false;
    message_latency.enabled = false;
    read_tests.enabled = false;
    read_complex_tests.enabled = false;
    write_tests.enabled = false;
    streaming_tests.enabled = false;
    scpi_compare_tests.enabled = false;
    uds_tests.enabled = false;

    async_init_tests.enabled = false;
    async_init_tests.command_counts = {2, 3, 5, 10};
    async_init_tests.iterations = 10000;

    parallel_stream_tests.enabled = false;
    parallel_stream_tests.client_counts = {2, 4, 8, 16};
    parallel_stream_tests.sample_counts = {1000, 10000, 100000, 200000};

    payload_write_tests.enabled = false;
    payload_write_tests.payload_sizes = {1, 8, 16, 32, 64, 128, 1024, 32768};
    payload_write_tests.output_prefix = "payloadlatency";

    payload_stream_tests.enabled = false;
    payload_stream_tests.payload_sizes = {1, 8, 16, 32, 64, 128, 1024, 32768};
    payload_stream_tests.output_prefix = "payloadstreamlatency";

    sideband_tests.enabled = false;
    sideband_tests.buffer_sizes = {4 * 1024 * 1024};
    sideband_tests.strategies = {"SOCKETS", "RDMA"};

    packing_tests.enabled = false;
    packing_tests.sample_counts = {1000, 10000, 100000};
    packing_tests.iteration_counts = {5000, 10000};

    single_latency_stream.enabled = true;
    single_latency_stream.filename = "streamlatency1.txt";
}

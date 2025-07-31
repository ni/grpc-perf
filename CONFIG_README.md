# Performance Test Configuration

The gRPC performance test client now supports configuration files to specify which tests to run, eliminating the need to modify and recompile the source code.

## Usage

### Basic Usage
```bash
# Run with default configuration (test_config.json)
./perftest_client

# Run with custom configuration file
./perftest_client --config my_tests.json

# Generate a default configuration file
./perftest_client --generate-config
```

### Configuration File Format

The configuration file uses JSON format and allows you to enable/disable various test suites and customize their parameters.

#### Test Suites

```json
{
    "test_suites": {
        "message_performance": true,     // Basic message performance tests
        "latency_stream": false,         // Streaming latency test suite
        "message_latency": false,        // Message latency test suite
        "read_tests": false,             // Read performance tests
        "read_complex_tests": false,     // Complex read tests with arenas
        "write_tests": false,            // Write performance tests
        "streaming_tests": false,        // Streaming performance tests
        "scpi_compare_tests": false,     // SCPI-like test comparison
        "uds_tests": false               // Unix Domain Socket tests (if enabled)
    }
}
```

#### Specialized Tests

##### Async Init Tests
```json
{
    "async_init_tests": {
        "enabled": false,
        "command_counts": [2, 3, 5, 10],  // Number of parallel commands to test
        "iterations": 10000               // Iterations per test
    }
}
```

##### Parallel Stream Tests
```json
{
    "parallel_stream_tests": {
        "enabled": false,
        "client_counts": [2, 4, 8, 16],           // Number of parallel clients
        "sample_counts": [1000, 10000, 100000]   // Sample counts to test
    }
}
```

##### Payload Tests
```json
{
    "payload_write_tests": {
        "enabled": false,
        "payload_sizes": [1, 8, 16, 32, 64, 128, 1024, 32768],  // Payload sizes in bytes
        "output_prefix": "payloadlatency"                        // Output file prefix
    },
    "payload_stream_tests": {
        "enabled": false,
        "payload_sizes": [1, 8, 16, 32, 64, 128, 1024, 32768],
        "output_prefix": "payloadstreamlatency"
    }
}
```

##### Sideband Tests (if GRPC_SIDEBAND enabled)
```json
{
    "sideband_tests": {
        "enabled": false,
        "buffer_sizes": [4194304],          // Buffer sizes in bytes (4MB default)
        "strategies": ["SOCKETS", "RDMA"]   // Available: SOCKETS, RDMA, SOCKETS_LOW_LATENCY, RDMA_LOW_LATENCY
    }
}
```

##### Single Latency Stream Test
```json
{
    "single_latency_stream": {
        "enabled": true,
        "filename": "streamlatency1.txt"    // Output filename
    }
}
```

##### Packing Tests
```json
{
    "packing_tests": {
        "enabled": false,
        "sample_counts": [1000, 10000, 100000],  // Number of samples
        "iteration_counts": [5000, 10000]        // Number of iterations
    }
}
```

## Example Configurations

### Minimal Configuration (Default)
Only runs basic message performance and single latency stream tests:
```json
{
    "test_suites": {
        "message_performance": true,
        "latency_stream": false,
        "message_latency": false,
        "read_tests": false,
        "read_complex_tests": false,
        "write_tests": false,
        "streaming_tests": false,
        "scpi_compare_tests": false,
        "uds_tests": false
    },
    "single_latency_stream": {
        "enabled": true,
        "filename": "streamlatency1.txt"
    }
}
```

### Comprehensive Testing
Enables most test suites for thorough performance analysis:
```json
{
    "test_suites": {
        "message_performance": true,
        "latency_stream": true,
        "message_latency": true,
        "read_tests": true,
        "write_tests": true,
        "streaming_tests": true
    },
    "async_init_tests": {
        "enabled": true,
        "command_counts": [2, 5, 10],
        "iterations": 5000
    },
    "payload_write_tests": {
        "enabled": true,
        "payload_sizes": [1, 16, 64, 1024]
    }
}
```

## Migration from Previous Version

Previously, you had to comment/uncomment test calls in the source code:
```cpp
// OLD WAY - modify source code
RunMessagePerformanceTestSuite(*client);
// PerformAsyncInitTest(*client, 2, 10000);  // commented out
```

Now you can achieve the same result with configuration:
```json
{
    "test_suites": {
        "message_performance": true
    },
    "async_init_tests": {
        "enabled": false
    }
}
```

## Command Line Options

- `--config <file>`: Specify configuration file (default: test_config.json)
- `--generate-config`: Generate a default configuration file and exit
- `--cert <file>`: Certificate file path
- `--target <address>`: Target server address (default: localhost)
- `--port <number>`: Target server port (default: 50051)
- `--help`: Show usage information

## Building

Make sure to build with the updated CMakeLists.txt that includes nlohmann/json dependency:

```bash
mkdir build && cd build
cmake ..
make
```

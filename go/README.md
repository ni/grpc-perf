# Go gRPC Performance Server

This is a Go implementation of the gRPC performance testing server, compatible with the existing C++, Rust, and .NET implementations in this repository.

## Prerequisites

- Go 1.18 or later
- Protocol Buffers compiler (`protoc`)
- Go protobuf plugins (automatically installed by setup script)

## Setup

1. **Install required tools:**
   ```bash
   make install-tools
   ```
   
   Or run the setup script directly:
   ```bash
   ./setup.sh
   ```

2. **Generate protobuf files:**
   ```bash
   make generate
   ```

3. **Download dependencies:**
   ```bash
   make deps
   ```

## Building

Build the server:
```bash
make build
```

This will create a binary at `bin/perftest_server`.

## Running

Start the server (TCP and UDS):
```bash
make server
```

Or run the binary directly:
```bash
./bin/perftest_server
```

The server will listen on:
- **TCP**: `0.0.0.0:50051` (default)
- **UDS**: `/tmp/perftest` (default, if enabled)

### Command Line Options

```bash
./bin/perftest_server [options]

Options:
  -address string      TCP address to bind to (default "0.0.0.0")
  -port string         TCP port to listen on (default "50051")
  -cert string         Path to certificate file for TLS
  -enable-uds          Enable Unix Domain Socket listener (default true)
  -uds-path string     Unix Domain Socket path (default "/tmp/perftest")
```

### Examples

```bash
# Default: TCP on 0.0.0.0:50051 and UDS on /tmp/perftest
./bin/perftest_server

# TCP only
./bin/perftest_server -enable-uds=false

# Custom TCP port
./bin/perftest_server -port 50052

# Custom UDS path
./bin/perftest_server -uds-path /tmp/my_perftest_socket

# TCP on different address
./bin/perftest_server -address 127.0.0.1 -port 8080
```

## Development

- **Format code:** `make fmt`
- **Run tests:** `make test`
- **Clean build:** `make clean`
- **Build for multiple platforms:** `make build-cross`

### Testing

Test with TCP connection:
```bash
make client
```

Test with UDS connection:
```bash
make client-uds
```

Run comprehensive integration tests (TCP + UDS):
```bash
make integration-test
```

## Architecture

The Go server implements two main services:

### 1. `NiPerfTestService`
Implements all the performance testing RPCs from `perftest.proto`:
- Streaming latency tests (bidirectional, client-streaming, server-streaming)
- Read operations (simple, complex, continuous)  
- Write operations (simple, continuous)
- Configuration RPCs
- Sideband streaming tests

### 2. `MonikerService`
Implements the data moniker service from `data_moniker.proto`:
- Sideband stream management
- Stream read/write operations

## Key Features

- **Concurrent safe:** Uses proper synchronization for shared state
- **Streaming support:** Full bidirectional, client, and server streaming
- **Compatible protocol:** Uses the same protobuf definitions as other implementations
- **Performance optimized:** Efficient memory allocation and minimal allocations in hot paths
- **Dual transport support:** Both TCP and Unix Domain Socket listeners
- **Extensible:** Easy to add new RPCs or modify behavior
- **Graceful shutdown:** Proper signal handling and resource cleanup

## Project Structure

```
go/
├── cmd/
│   └── server/          # Server main application
│       └── main.go
├── pkg/
│   ├── perftest/        # Generated perftest protobuf files
│   └── datamoniker/     # Generated data moniker protobuf files
├── bin/                 # Built binaries (created during build)
├── Makefile            # Build automation
├── go.mod              # Go module definition
├── generate.sh         # Protobuf generation script
├── setup.sh           # Tool installation script
└── README.md          # This file
```

## Compatibility

This Go implementation is fully compatible with:
- The C++ client and server implementations
- The Rust server implementation  
- The .NET server implementation
- The Python client implementations

All implementations use the same protobuf service definitions and should be interchangeable for performance testing.

## Performance Notes

The Go implementation includes several optimizations:
- Pre-allocated slices for repeated data
- Efficient streaming with proper EOF handling
- Minimal memory allocations in hot paths
- Concurrent handling of multiple client connections

For CPU affinity features (like in the C++ implementation), additional libraries or system calls would be needed, but the basic performance characteristics should be comparable.
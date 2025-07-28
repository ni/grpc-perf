# Rust gRPC Performance Test Server

This directory contains a Rust implementation of the gRPC performance test server.

## Prerequisites

- Rust 1.70+ (install via [rustup](https://rustup.rs/))
- Protocol Buffers compiler (`protoc`)

On Ubuntu/Debian:
```bash
sudo apt update
sudo apt install protobuf-compiler
```

On macOS:
```bash
brew install protobuf
```

## Building

From the `rust/` directory:

```bash
cargo build --release
```

## Running

Start the server:

```bash
cargo run --release --bin perftest_server
```

Or run the compiled binary:

```bash
./target/release/perftest_server
```

## Command Line Options

```bash
cargo run --release --bin perftest_server -- --help
```

Available options:
- `--cert <path>`: Path to certificate file for TLS (optional)
- `--port <port>`: Port to listen on (default: 50051)
- `--address <addr>`: Address to bind to (default: 0.0.0.0)

## Examples

Run on default port (50051):
```bash
cargo run --release
```

Run on custom port:
```bash
cargo run --release -- --port 8080
```

Run with specific bind address:
```bash
cargo run --release -- --address 127.0.0.1 --port 50051
```

## Implementation Details

The Rust server implements all the same gRPC service methods as the C++ and C# versions:

### niPerfTestService
- `StreamLatencyTest` - Bidirectional streaming latency test
- `StreamLatencyTestClient` - Client streaming to unary
- `StreamLatencyTestServer` - Unary to server streaming  
- `TestWrite` - Simple unary write test
- `TestWriteContinuously` - Bidirectional streaming write test
- `Init` - Initialization method
- `ConfigureVertical` - Vertical configuration
- `ConfigureHorizontalTiming` - Horizontal timing configuration
- `InitiateAcquisition` - Acquisition initiation
- `Read` - Read samples
- `ReadComplex` - Read complex number samples
- `ReadComplexArena` - Read complex samples (arena allocated)
- `ReadContinuously` - Streaming read

### MonikerService
- `BeginSidebandStream` - Begin sideband streaming
- `StreamReadWrite` - Bidirectional read/write streaming
- `StreamRead` - Server streaming read
- `StreamWrite` - Client streaming write

## Performance

The Rust implementation uses:
- [Tonic](https://github.com/hyperium/tonic) - A native gRPC implementation for Rust
- [Tokio](https://tokio.rs/) - Async runtime
- Zero-copy where possible
- Async/await for all I/O operations

For optimal performance, always build and run with `--release` flag.

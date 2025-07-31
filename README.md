# gRPC performance test application

This project supports Windows, Linux and Linux RT for both the client and server.
Always build release when running benchmarks. There is a large difference in performance between debug and release.

## Available Implementations

This repository contains gRPC performance test implementations in multiple languages:

- **C++** (`src/`) - Primary implementation with client and server
- **C#/.NET** (`dotnet/`) - Server implementation using ASP.NET Core
- **Python** (`python/`) - Client implementation 
- **Rust** (`rust/`) - High-performance server implementation using Tonic
- **LabVIEW** (`labview/`) - LabVIEW integration components

All server implementations are compatible with the C++ client for cross-language performance testing.

## Building on Windows

### Prerequisites
To prepare for cmake + Microsoft Visual C++ compiler build
- Install Visual Studio 2015, 2017, or 2019 (Visual C++ compiler will be used).
- Install [Git](https://git-scm.com/).
- Install gRPC for C++
- Install [CMake](https://cmake.org/download/).


### Building
- Launch "x64 Native Tools Command Prompt for Visual Studio"

Download the repo and update submodules, this will pull the gRPC components and all dependencies

```
> git clone https://github.com/ni/grpc-perf grpc-perf
> cd grpc-perf
> git submodule update --init --recursive
```

Build Debug - Do not build debug for profiling
```
> mkdir build
> cd build
> cmake ..
> cmake --build .
```

Build Release
```
> mkdir build
> cd build
> cmake ..
> cmake --build . --config Release
```

## Building on Linux

Download the repo and update submodules, this will pull the gRPC components and all dependencies

```
> git clone https://github.com/ni/grpc-perf grpc-perf
> cd grpc-perf
> git submodule update --init --recursive
```

Build - Do not build debug for profiling

```
> mkdir -p cmake/build
> cd cmake/build
> cmake ../..
> make
```

Build Release

```
> mkdir -p cmake/build
> cd cmake/build
> cmake ../..
> cmake -DCMAKE_BUILD_TYPE=Release ../..
> make
```

## Building on Linux RT

Install required packages not installed by default

```
> opkg update
> opkg install git
> opkg install git-perltools
> opkg install cmake
> opkg install g++
> opkg install g++-symlinks
```

Download the repo and update submodules, this will pull the gRPC components and all dependencies

```
> git clone https://github.com/ni/grpc-perf grpc-perf
> cd grpc-perf
> git submodule update --init --recursive
```

Build Debug - Do not build debug for profiling

```
> mkdir -p cmake/build
> cd cmake/build
> cmake ../..
> make
```

Build Release

```
> mkdir -p cmake/build
> cd cmake/build
> cmake -DCMAKE_BUILD_TYPE=Release ../..
> make
```

## Building Rust Server

The Rust implementation provides a high-performance gRPC server implementation.

### Prerequisites
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

### Building
From the `rust/` directory:

```bash
cd rust
cargo build --release
```

Or use the provided build script:
```bash
cd rust
./build.sh
```

### Running
```bash
cd rust
./target/release/perftest_server
```

Run with custom options:
```bash
cd rust
./target/release/perftest_server --port 8080 --address 127.0.0.1
```

For help:
```bash
cd rust
./target/release/perftest_server --help
```

## Running Tests

### New Configuration System ⭐

The performance test client now supports JSON configuration files to specify which tests to run, eliminating the need to modify and recompile source code.

**Quick Configuration Setup:**
```bash
# Generate a default configuration file
./perftest_client --generate-config

# Edit test_config.json to enable/disable desired tests
# Run with the configuration
./perftest_client

# Or specify a custom config file
./perftest_client --config my_custom_tests.json
```

**Command Line Options:**
- `--config <file>`: Specify configuration file (default: test_config.json)
- `--generate-config`: Generate a default configuration file and exit
- `--cert <file>`: Certificate file path for secure connections
- `--target <address>`: Target server address (default: localhost)
- `--port <number>`: Target server port (default: 50051)

For detailed configuration options and examples, see [CONFIG_README.md](CONFIG_README.md).

### Traditional Usage

Start the server, perftest_server, on the server machine.
Run the client, perftest_client.
If you want to run the client on a different machine pass in the server to conect to: perftest_client --target={server name or ip}

## Generating Python

```
pip install grpcio-tools
```

```
python -m grpc_tools.protoc -I="..\.." --python_out=. --grpc_python_out=. perftest.proto
```

## SSL/TLS Support

You can enable SSL/TLS support on the server by passing in a path to a certificate and private key for the server.

You can generate the certificate with openssl using the following script.

```
mypass="password123"

echo Generate server key:
openssl genrsa -passout pass:$mypass -des3 -out server.key 4096

echo Generate server signing request:
openssl req -passin pass:$mypass -new -key server.key -out server.csr -subj  "/C=US/ST=TX/L=Austin/O=NI/OU=perftest/CN=localhost"

echo Self-sign server certificate:
openssl x509 -req -passin pass:$mypass -days 365 -in server.csr -signkey server.key -set_serial 01 -out server.crt

echo Remove passphrase from server key:
openssl rsa -passin pass:$mypass -in server.key -out server.key

rm server.csr
```

Clients then must connect using the server certificate that was generated (server.cer) otherwise the connection will fail.

If you do not passing in a certificate then the server will use insecure gRPC.

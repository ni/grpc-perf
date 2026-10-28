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

## Network Tuning for Low Latency

Default operating system and network card settings favor throughput and power savings over latency.
For small request/response messages between two machines, tuning both ends commonly cuts round-trip latency by 3-5x and removes most outliers.
Always build Release, and change one setting at a time so the effect of each can be measured.

### Measuring

Build Release with sideband support: add `-DINCLUDE_GRPC_SIDEBAND=ON` to the cmake configure command (and `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` when using CMake 4.x).

The sideband latency test (`SOCKETS_LOW_LATENCY`) measures round trips of a small (<100 byte) message over a TCP sideband socket.
The client reports min, max, median, average, P90, P99 and P99.9 in microseconds, and writes every sample to `SidebandLatency.txt`.

When the client is on a different machine, start the server with the address clients should use to reach the sideband socket (the default advertises `localhost`):

```
./perftest_server --sideband-address <server ip>
```

On Windows, `scripts/profile_sideband_latency.ps1` runs the test with `test_config_sideband_latency.json` (200,000 iterations):

```
.\scripts\profile_sideband_latency.ps1 -Target <server ip>          # remote server
.\scripts\profile_sideband_latency.ps1                              # starts a local server
.\scripts\profile_sideband_latency.ps1 -Target <server ip> -Trace   # also records a WPR trace (elevated)
```

Note: do not probe the sideband port (50055) with a plain TCP connect; the sideband listener hangs if a connection closes before sending its ID.

### Network path

- Use a direct cable between the two machines where possible. Removing the switch reduced the median round trip by roughly a third in our measurements.
- Avoid USB network adapters. They batch packets in hardware and added more than 1 ms per round trip.
- Link speed matters little for small messages (serializing a 100 byte frame takes 0.8 us at 1 Gbps), so a well-tuned 1 Gbps link can beat an untuned 10 Gbps link.
- On multi-homed machines, advertise the sideband address on the interface the server actually routes to the client through (`ip route get <client ip>` on Linux).

### Windows

Settings are per network adapter and persist across reboots. Each change briefly resets the adapter. Run from an elevated PowerShell:

```powershell
# Show the current settings; names and values vary by driver
Get-NetAdapterAdvancedProperty -Name "<adapter>" | Where-Object DisplayName -match 'Interrupt|Energy|Power'

Set-NetAdapterAdvancedProperty -Name "<adapter>" -DisplayName "Interrupt Moderation" -DisplayValue "Disabled"
Set-NetAdapterAdvancedProperty -Name "<adapter>" -DisplayName "Energy-Efficient Ethernet" -DisplayValue "Disabled"   # Intel: "Energy Efficient Ethernet" / "Off"
Set-NetAdapterAdvancedProperty -Name "<adapter>" -DisplayName "PCI Express Link Power Saving" -DisplayValue "Disabled" # if present
```

- Interrupt moderation is the most important setting: when adaptive, it delays received packets on a timer, producing latency clusters roughly 100 us apart.
- Use the "Ultimate Performance" or "High Performance" power plan.
- Run the client elevated so it can use the `REALTIME` priority class; otherwise it falls back to `HIGH`.

### Linux

These settings reset on reboot. Persist the sysctls in `/etc/sysctl.d/`, and apply the `ethtool` and `/sys` settings from a startup script or your network manager.

```bash
# Busy-poll sockets instead of sleeping. The value (us) must cover the gap between
# messages; 50 was too short for a ~100 us round trip, 200 removed the slow mode.
sudo sysctl -w net.core.busy_read=200 net.core.busy_poll=200

# Disable interrupt coalescing (supported by most server NICs, e.g. ConnectX)
sudo ethtool -C <iface> adaptive-rx off adaptive-tx off rx-usecs 0 rx-frames 1 tx-usecs 0 tx-frames 1

# Disable Energy Efficient Ethernet
sudo ethtool --set-eee <iface> eee off

# Disable NAPI deferral / GRO flush delay
echo 0 | sudo tee /sys/class/net/<iface>/napi_defer_hard_irqs /sys/class/net/<iface>/gro_flush_timeout
```

- Use a latency-oriented power profile (e.g. `latency-performance`) and the `performance` CPU frequency governor.
- Optionally disable deep CPU idle states to avoid wake-up latency: `echo 1 | sudo tee /sys/devices/system/cpu/cpu*/cpuidle/state{1,2,3}/disable`.
- Run the server as a normal user. Running it with `SCHED_FIFO` priority (as root) made latency worse, likely by starving the kernel's network softirq processing.
- Running the server as root leaves a root-owned `/tmp/perftest` socket; remove it (`sudo rm /tmp/perftest`) before running as a normal user again.

### Example results

Windows client to an NVIDIA DGX Spark server, `SOCKETS_LOW_LATENCY`, 200,000 iterations, microseconds:

| Path | Min | Median | Avg | P90 | P99 | P99.9 | Max |
|---|---|---|---|---|---|---|---|
| Before NIC tuning, 1 Gbps via switch | 91.0 | 314.0 | 341.0 | 458.0 | 700.0 | 13,492 | 29,765 |
| Tuned, 1 Gbps via switch | 68.8 | 103.9 | 109.0 | 107.2 | 145.2 | 734.7 | 26,389 |
| Tuned, 1 Gbps direct link | 39.4 | 66.0 | 69.1 | 73.3 | 90.8 | 184.0 | 22,305 |

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

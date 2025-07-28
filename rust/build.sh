#!/bin/bash

# Simple build script for the Rust gRPC performance test server

set -e

echo "Building Rust gRPC performance test server..."

# Build in debug mode
echo "Building debug version..."
cargo build

# Build in release mode
echo "Building release version..."
cargo build --release

echo "Build complete!"
echo "Debug binary: ./target/debug/perftest_server"
echo "Release binary: ./target/release/perftest_server"

echo ""
echo "Usage:"
echo "  Debug:   ./target/debug/perftest_server"
echo "  Release: ./target/release/perftest_server"
echo "  Help:    ./target/release/perftest_server --help"

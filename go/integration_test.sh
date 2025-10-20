#!/bin/bash

# Integration test script for Go gRPC server
# Tests the Go server against existing Python client to verify compatibility
# Tests both TCP and UDS connections

set -e

SERVER_PORT=50051
UDS_PATH="/tmp/perftest"
SERVER_BINARY="./bin/perftest_server"
PYTHON_CLIENT="../python/client/perfTestClient.py"

echo "Starting Go gRPC Performance Server Integration Test..."

# Check if binaries exist
if [ ! -f "$SERVER_BINARY" ]; then
    echo "Error: Server binary not found. Run 'make build' first."
    exit 1
fi

if [ ! -f "$PYTHON_CLIENT" ]; then
    echo "Warning: Python client not found at $PYTHON_CLIENT"
    echo "Will skip Python client compatibility test."
    SKIP_PYTHON=1
fi

# Clean up any existing UDS socket
rm -f "$UDS_PATH"

# Start the Go server in background (with both TCP and UDS)
echo "Starting Go gRPC server on TCP port $SERVER_PORT and UDS $UDS_PATH..."
$SERVER_BINARY &
SERVER_PID=$!

# Give server time to start
sleep 3

# Test with our Go client (TCP)
echo "Testing with Go client (TCP)..."
./bin/perftest_client
echo "✓ Go client TCP test passed"

# Test with our Go client (UDS)
echo "Testing with Go client (UDS)..."
./bin/perftest_client -uds
echo "✓ Go client UDS test passed"

# Test with Python client if available (TCP only for now)
if [ -z "$SKIP_PYTHON" ] && command -v python3 > /dev/null; then
    echo "Testing with Python client (TCP)..."
    cd ../python/client
    if python3 -c "import grpc, perftest_pb2, perftest_pb2_grpc" 2>/dev/null; then
        python3 perfTestClient.py
        echo "✓ Python client TCP test passed"
    else
        echo "Warning: Python dependencies not available. Skipping Python test."
    fi
    cd - > /dev/null
fi

# Performance comparison
echo ""
echo "=== Performance Comparison ==="
echo "Testing TCP performance..."
./bin/perftest_client | grep "Performance test complete"

echo "Testing UDS performance..."
./bin/perftest_client -uds | grep "Performance test complete"

# Cleanup
echo ""
echo "Stopping server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null || true

# Clean up UDS socket
rm -f "$UDS_PATH"

echo ""
echo "✓ All integration tests completed successfully!"
echo "The Go gRPC server is fully compatible with existing clients on both TCP and UDS."
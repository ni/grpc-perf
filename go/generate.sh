#!/bin/bash

# Generate Go protobuf and gRPC code from proto files

set -e

# Ensure we're in the go directory
cd "$(dirname "$0")"

# Create output directories if they don't exist
mkdir -p pkg/perftest
mkdir -p pkg/datamoniker

# Generate perftest protobuf files
protoc --go_out=pkg/perftest --go_opt=paths=source_relative \
    --go-grpc_out=pkg/perftest --go-grpc_opt=paths=source_relative \
    --go_opt=Mperftest.proto=github.com/ni/grpc-perf/go/pkg/perftest \
    --go_opt=Mdata_moniker.proto=github.com/ni/grpc-perf/go/pkg/datamoniker \
    --go-grpc_opt=Mperftest.proto=github.com/ni/grpc-perf/go/pkg/perftest \
    --go-grpc_opt=Mdata_moniker.proto=github.com/ni/grpc-perf/go/pkg/datamoniker \
    -I=.. \
    -I=../google/protobuf \
    ../perftest.proto

# Generate data_moniker protobuf files  
protoc --go_out=pkg/datamoniker --go_opt=paths=source_relative \
    --go-grpc_out=pkg/datamoniker --go-grpc_opt=paths=source_relative \
    --go_opt=Mdata_moniker.proto=github.com/ni/grpc-perf/go/pkg/datamoniker \
    --go-grpc_opt=Mdata_moniker.proto=github.com/ni/grpc-perf/go/pkg/datamoniker \
    -I=.. \
    -I=../google/protobuf \
    ../data_moniker.proto

echo "Protocol buffer files generated successfully"
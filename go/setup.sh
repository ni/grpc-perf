#!/bin/bash

# Setup script to install required Go protobuf tools

set -e

echo "Installing Go protobuf and gRPC plugins..."

# Install protoc-gen-go for generating protobuf Go code (compatible with Go 1.18)
go install google.golang.org/protobuf/cmd/protoc-gen-go@v1.28.1

# Install protoc-gen-go-grpc for generating gRPC Go code (compatible with Go 1.18)
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@v1.2.0

echo "Go protobuf tools installed successfully"
echo "Make sure \$GOPATH/bin or \$HOME/go/bin is in your PATH"
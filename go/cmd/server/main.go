package main

import (
	"context"
	"flag"
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"os/signal"
	"sync"
	"syscall"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/credentials/insecure"

	"github.com/ni/grpc-perf/go/pkg/perftest"
	"github.com/ni/grpc-perf/go/pkg/datamoniker"
)

// PerfTestServer implements the niPerfTestService
type PerfTestServer struct {
	perftest.UnimplementedNiPerfTestServiceServer
	writers map[uint32]perftest.NiPerfTestService_StreamLatencyTestServerServer
	mu      sync.RWMutex
}

func NewPerfTestServer() *PerfTestServer {
	return &PerfTestServer{
		writers: make(map[uint32]perftest.NiPerfTestService_StreamLatencyTestServerServer),
	}
}

// StreamLatencyTest implements bidirectional streaming for latency testing
func (s *PerfTestServer) StreamLatencyTest(stream perftest.NiPerfTestService_StreamLatencyTestServer) error {
	for {
		client, err := stream.Recv()
		if err == io.EOF {
			return nil
		}
		if err != nil {
			return err
		}

		server := &perftest.StreamLatencyServer{
			Message: client.Message,
		}

		if err := stream.Send(server); err != nil {
			return err
		}
	}
}

// StreamLatencyTestClient implements client streaming latency test
func (s *PerfTestServer) StreamLatencyTestClient(stream perftest.NiPerfTestService_StreamLatencyTestClientServer) error {
	var lastMessage uint32
	var slot uint32
	first := true

	for {
		client, err := stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			return err
		}

		slot = client.Message
		lastMessage = client.Message

		if first {
			first = false
			// In Go, we can't set CPU affinity as easily as in C++
			// This would require using syscalls or third-party libraries
		}

		s.mu.RLock()
		writer := s.writers[slot]
		s.mu.RUnlock()

		if writer != nil {
			server := &perftest.StreamLatencyServer{
				Message: lastMessage,
			}
			if err := writer.Send(server); err != nil {
				log.Printf("Error sending to writer: %v", err)
			}
		}
	}

	s.mu.Lock()
	delete(s.writers, slot)
	s.mu.Unlock()

	return stream.SendAndClose(&perftest.StreamLatencyServer{
		Message: lastMessage,
	})
}

// StreamLatencyTestServer implements server streaming latency test  
func (s *PerfTestServer) StreamLatencyTestServer(request *perftest.StreamLatencyClient, stream perftest.NiPerfTestService_StreamLatencyTestServerServer) error {
	slot := request.Message

	s.mu.Lock()
	s.writers[slot] = stream
	s.mu.Unlock()

	// Keep the stream alive until it's removed by StreamLatencyTestClient
	select {
	case <-stream.Context().Done():
		s.mu.Lock()
		delete(s.writers, slot)
		s.mu.Unlock()
		return stream.Context().Err()
	}
}

// TestWrite implements the TestWrite RPC
func (s *PerfTestServer) TestWrite(ctx context.Context, req *perftest.TestWriteParameters) (*perftest.TestWriteResult, error) {
	return &perftest.TestWriteResult{
		Status: 0,
	}, nil
}

// TestWriteContinuously implements bidirectional streaming for continuous write testing
func (s *PerfTestServer) TestWriteContinuously(stream perftest.NiPerfTestService_TestWriteContinuouslyServer) error {
	for {
		_, err := stream.Recv()
		if err == io.EOF {
			return nil
		}
		if err != nil {
			return err
		}

		result := &perftest.TestWriteResult{
			Status: 0,
		}

		if err := stream.Send(result); err != nil {
			return err
		}
	}
}

// BeginTestSidebandStream implements sideband stream initialization
func (s *PerfTestServer) BeginTestSidebandStream(ctx context.Context, req *perftest.BeginTestSidebandStreamRequest) (*perftest.BeginTestSidebandStreamResponse, error) {
	return &perftest.BeginTestSidebandStreamResponse{
		Strategy:           req.Strategy,
		ConnectionUrl:      "localhost:50052",
		SidebandIdentifier: "go_sideband_001",
	}, nil
}

// TestSidebandStream implements sideband streaming
func (s *PerfTestServer) TestSidebandStream(stream perftest.NiPerfTestService_TestSidebandStreamServer) error {
	for {
		_, err := stream.Recv()
		if err == io.EOF {
			return nil
		}
		if err != nil {
			return err
		}

		response := &perftest.TestSidebandStreamResponse{
			SidebandLocation: "localhost:50052",
		}

		if err := stream.Send(response); err != nil {
			return err
		}
	}
}

// Init implements the Init RPC
func (s *PerfTestServer) Init(ctx context.Context, req *perftest.InitParameters) (*perftest.InitResult, error) {
	return &perftest.InitResult{
		Status: req.Id,
	}, nil
}

// ConfigureVertical implements the ConfigureVertical RPC
func (s *PerfTestServer) ConfigureVertical(ctx context.Context, req *perftest.ConfigureVerticalRequest) (*perftest.ConfigureVerticalResponse, error) {
	return &perftest.ConfigureVerticalResponse{
		Status: 0,
	}, nil
}

// ConfigureHorizontalTiming implements the ConfigureHorizontalTiming RPC
func (s *PerfTestServer) ConfigureHorizontalTiming(ctx context.Context, req *perftest.ConfigureHorizontalTimingRequest) (*perftest.ConfigureHorizontalTimingResponse, error) {
	return &perftest.ConfigureHorizontalTimingResponse{
		Status: 0,
	}, nil
}

// InitiateAcquisition implements the InitiateAcquisition RPC
func (s *PerfTestServer) InitiateAcquisition(ctx context.Context, req *perftest.InitiateAcquisitionRequest) (*perftest.InitiateAcquisitionResponse, error) {
	return &perftest.InitiateAcquisitionResponse{
		Status: 0,
	}, nil
}

// Read implements the Read RPC
func (s *PerfTestServer) Read(ctx context.Context, req *perftest.ReadParameters) (*perftest.ReadResult, error) {
	samples := make([]float64, req.NumSamples)
	for i := range samples {
		samples[i] = 8.325793493
	}

	return &perftest.ReadResult{
		Status:  0,
		Samples: samples,
	}, nil
}

// ReadComplex implements the ReadComplex RPC
func (s *PerfTestServer) ReadComplex(ctx context.Context, req *perftest.ReadParameters) (*perftest.ReadComplexResult, error) {
	samples := make([]*perftest.ComplexNumber, req.NumSamples)
	for i := range samples {
		samples[i] = &perftest.ComplexNumber{
			Real:      3.14,
			Imaginary: 4.56,
		}
	}

	return &perftest.ReadComplexResult{
		Status:  0,
		Samples: samples,
	}, nil
}

// ReadComplexArena implements the ReadComplexArena RPC (same as ReadComplex for simplicity)
func (s *PerfTestServer) ReadComplexArena(ctx context.Context, req *perftest.ReadParameters) (*perftest.ReadComplexResult, error) {
	return s.ReadComplex(ctx, req)
}

// ReadContinuously implements server streaming for continuous reads
func (s *PerfTestServer) ReadContinuously(req *perftest.ReadContinuouslyParameters, stream perftest.NiPerfTestService_ReadContinuouslyServer) error {
	wfm := make([]float64, req.NumSamples)
	for i := range wfm {
		wfm[i] = 0.0
	}

	iterations := req.NumIterations
	if iterations == 0 {
		iterations = 10000
	}

	for i := int32(0); i < iterations; i++ {
		result := &perftest.ReadContinuouslyResult{
			Status: 0,
			Wfm:    wfm,
		}

		if err := stream.Send(result); err != nil {
			return err
		}
	}

	return nil
}

// MonikerServer implements the MonikerService
type MonikerServer struct {
	datamoniker.UnimplementedMonikerServiceServer
}

func NewMonikerServer() *MonikerServer {
	return &MonikerServer{}
}

// BeginSidebandStream implements moniker sideband stream initialization
func (s *MonikerServer) BeginSidebandStream(ctx context.Context, req *datamoniker.BeginMonikerSidebandStreamRequest) (*datamoniker.BeginMonikerSidebandStreamResponse, error) {
	return &datamoniker.BeginMonikerSidebandStreamResponse{
		Strategy:           req.Strategy,
		ConnectionUrl:      "localhost:50053",
		SidebandIdentifier: "go_moniker_001",
		BufferSize:         1024 * 1024,
	}, nil
}

// StreamReadWrite implements bidirectional streaming for moniker read/write
func (s *MonikerServer) StreamReadWrite(stream datamoniker.MonikerService_StreamReadWriteServer) error {
	for {
		_, err := stream.Recv()
		if err == io.EOF {
			return nil
		}
		if err != nil {
			return err
		}

		result := &datamoniker.MonikerReadResult{
			Data: nil,
		}

		if err := stream.Send(result); err != nil {
			return err
		}
	}
}

// StreamRead implements server streaming for moniker reads
func (s *MonikerServer) StreamRead(req *datamoniker.MonikerList, stream datamoniker.MonikerService_StreamReadServer) error {
	result := &datamoniker.MonikerReadResult{
		Data: nil,
	}

	return stream.Send(result)
}

// StreamWrite implements bidirectional streaming for moniker writes
func (s *MonikerServer) StreamWrite(stream datamoniker.MonikerService_StreamWriteServer) error {
	for {
		_, err := stream.Recv()
		if err == io.EOF {
			return nil
		}
		if err != nil {
			return err
		}

		response := &datamoniker.StreamWriteResponse{}

		if err := stream.Send(response); err != nil {
			return err
		}
	}
}

func createCredentials(certPath string) (credentials.TransportCredentials, error) {
	if certPath == "" {
		return insecure.NewCredentials(), nil
	}

	// For TLS, you would load certificates here
	// For now, just return insecure credentials
	return insecure.NewCredentials(), nil
}

func main() {
	// Parse command line arguments
	var (
		address    = flag.String("address", "0.0.0.0", "TCP address to bind to")
		port       = flag.String("port", "50051", "TCP port to listen on")
		certPath   = flag.String("cert", "", "Path to certificate file for TLS")
		enableUDS  = flag.Bool("enable-uds", true, "Enable Unix Domain Socket listener")
		udsPath    = flag.String("uds-path", "/tmp/perftest", "Unix Domain Socket path")
	)
	flag.Parse()

	// Create credentials
	creds, err := createCredentials(*certPath)
	if err != nil {
		log.Fatalf("Failed to create credentials: %v", err)
	}

	// Create gRPC server options
	opts := []grpc.ServerOption{
		grpc.Creds(creds),
		grpc.MaxRecvMsgSize(1 * 1024 * 1024),
		grpc.MaxSendMsgSize(1 * 1024 * 1024),
	}

	server := grpc.NewServer(opts...)

	// Register services
	perfTestServer := NewPerfTestServer()
	monikerServer := NewMonikerServer()
	
	perftest.RegisterNiPerfTestServiceServer(server, perfTestServer)
	datamoniker.RegisterMonikerServiceServer(server, monikerServer)

	// Create a context that can be cancelled
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Set up signal handling
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	var wg sync.WaitGroup

	// Start TCP listener
	tcpAddress := fmt.Sprintf("%s:%s", *address, *port)
	wg.Add(1)
	go func() {
		defer wg.Done()
		if err := serveTCP(ctx, server, tcpAddress); err != nil {
			log.Printf("TCP server error: %v", err)
		}
	}()

	// Start UDS listener if enabled
	if *enableUDS {
		wg.Add(1)
		go func() {
			defer wg.Done()
			if err := serveUDS(ctx, server, *udsPath); err != nil {
				log.Printf("UDS server error: %v", err)
			}
		}()
	}

	// Wait for shutdown signal
	go func() {
		<-sigChan
		log.Println("Received shutdown signal, stopping servers...")
		server.GracefulStop()
		cancel()
	}()

	fmt.Printf("Go gRPC server listening on TCP: %s", tcpAddress)
	if *enableUDS {
		fmt.Printf(" and UDS: %s", *udsPath)
	}
	fmt.Println()

	// Wait for all servers to finish
	wg.Wait()
	log.Println("Server shutdown complete")
}

// serveTCP starts the TCP listener
func serveTCP(ctx context.Context, server *grpc.Server, address string) error {
	listener, err := net.Listen("tcp", address)
	if err != nil {
		return fmt.Errorf("failed to listen on TCP %s: %v", address, err)
	}

	log.Printf("TCP server listening on %s", address)
	
	go func() {
		<-ctx.Done()
		listener.Close()
	}()

	if err := server.Serve(listener); err != nil {
		return fmt.Errorf("TCP server failed: %v", err)
	}
	return nil
}

// serveUDS starts the Unix Domain Socket listener
func serveUDS(ctx context.Context, server *grpc.Server, socketPath string) error {
	// Remove existing socket file if it exists
	if err := os.RemoveAll(socketPath); err != nil {
		return fmt.Errorf("failed to remove existing socket file %s: %v", socketPath, err)
	}

	listener, err := net.Listen("unix", socketPath)
	if err != nil {
		return fmt.Errorf("failed to listen on UDS %s: %v", socketPath, err)
	}

	// Set socket permissions to allow access
	if err := os.Chmod(socketPath, 0666); err != nil {
		log.Printf("Warning: failed to set socket permissions: %v", err)
	}

	log.Printf("UDS server listening on %s", socketPath)

	go func() {
		<-ctx.Done()
		listener.Close()
		os.RemoveAll(socketPath) // Clean up socket file on shutdown
	}()

	if err := server.Serve(listener); err != nil {
		return fmt.Errorf("UDS server failed: %v", err)
	}
	return nil
}
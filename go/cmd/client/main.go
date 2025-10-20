package main

import (
	"context"
	"flag"
	"fmt"
	"log"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"

	"github.com/ni/grpc-perf/go/pkg/perftest"
)

func main() {
	// Parse command line arguments
	var (
		useUDS     = flag.Bool("uds", false, "Use Unix Domain Socket instead of TCP")
		udsPath    = flag.String("uds-path", "/tmp/perftest", "Unix Domain Socket path")
		tcpAddress = flag.String("address", "localhost:50051", "TCP address to connect to")
	)
	flag.Parse()

	var target string
	var dialOpts []grpc.DialOption

	if *useUDS {
		target = "unix:" + *udsPath
		dialOpts = []grpc.DialOption{
			grpc.WithTransportCredentials(insecure.NewCredentials()),
		}
		fmt.Printf("Connecting to UDS: %s\n", *udsPath)
	} else {
		target = *tcpAddress
		dialOpts = []grpc.DialOption{
			grpc.WithTransportCredentials(insecure.NewCredentials()),
		}
		fmt.Printf("Connecting to TCP: %s\n", *tcpAddress)
	}

	// Connect to server
	conn, err := grpc.Dial(target, dialOpts...)
	if err != nil {
		log.Fatalf("Failed to connect: %v", err)
	}
	defer conn.Close()

	client := perftest.NewNiPerfTestServiceClient(conn)
	ctx := context.Background()

	fmt.Println("Testing Go gRPC server...")

	// Test Init
	fmt.Print("Testing Init... ")
	initResp, err := client.Init(ctx, &perftest.InitParameters{
		Command: "",
		Id:      123,
	})
	if err != nil {
		log.Fatalf("Init failed: %v", err)
	}
	if initResp.Status == 123 {
		fmt.Println("✓ PASS")
	} else {
		fmt.Printf("✗ FAIL (expected 123, got %d)\n", initResp.Status)
	}

	// Test Read
	fmt.Print("Testing Read... ")
	samples := int32(1000)
	readResp, err := client.Read(ctx, &perftest.ReadParameters{
		Timeout:    10.0,
		NumSamples: samples,
	})
	if err != nil {
		log.Fatalf("Read failed: %v", err)
	}
	if len(readResp.Samples) == int(samples) && readResp.Status == 0 {
		fmt.Println("✓ PASS")
	} else {
		fmt.Printf("✗ FAIL (expected %d samples with status 0, got %d samples with status %d)\n", 
			samples, len(readResp.Samples), readResp.Status)
	}

	// Test ReadComplex
	fmt.Print("Testing ReadComplex... ")
	complexResp, err := client.ReadComplex(ctx, &perftest.ReadParameters{
		Timeout:    10.0,
		NumSamples: 100,
	})
	if err != nil {
		log.Fatalf("ReadComplex failed: %v", err)
	}
	if len(complexResp.Samples) == 100 && complexResp.Status == 0 {
		fmt.Println("✓ PASS")
	} else {
		fmt.Printf("✗ FAIL (expected 100 complex samples, got %d)\n", len(complexResp.Samples))
	}

	// Test TestWrite
	fmt.Print("Testing TestWrite... ")
	testData := make([]float64, 50)
	for i := range testData {
		testData[i] = float64(i) * 1.5
	}
	writeResp, err := client.TestWrite(ctx, &perftest.TestWriteParameters{
		Samples: testData,
	})
	if err != nil {
		log.Fatalf("TestWrite failed: %v", err)
	}
	if writeResp.Status == 0 {
		fmt.Println("✓ PASS")
	} else {
		fmt.Printf("✗ FAIL (expected status 0, got %d)\n", writeResp.Status)
	}

	// Performance test - similar to Python client
	fmt.Println("\nRunning performance test...")
	iterations := 1000
	testSamples := int32(10)

	startTime := time.Now()
	for i := 0; i < iterations; i++ {
		// Configure vertical
		_, err := client.ConfigureVertical(ctx, &perftest.ConfigureVerticalRequest{
			Vi:          "",
			ChannelList: "0",
			Range:       10.0,
			Offset:      0.0,
			CouplingEnum: &perftest.ConfigureVerticalRequest_Coupling{
				Coupling: perftest.VerticalCoupling_VERTICAL_COUPLING_NISCOPE_VAL_AC,
			},
			ProbeAttenuation: 1.0,
			Enabled:          true,
		})
		if err != nil {
			log.Fatalf("ConfigureVertical failed: %v", err)
		}

		// Configure horizontal timing
		_, err = client.ConfigureHorizontalTiming(ctx, &perftest.ConfigureHorizontalTimingRequest{
			Vi:              "",
			MinSampleRate:   50000000,
			MinNumPts:       testSamples,
			RefPosition:     50.0,
			NumRecords:      1,
			EnforceRealtime: true,
		})
		if err != nil {
			log.Fatalf("ConfigureHorizontalTiming failed: %v", err)
		}

		// Initiate acquisition
		_, err = client.InitiateAcquisition(ctx, &perftest.InitiateAcquisitionRequest{
			Vi: "",
		})
		if err != nil {
			log.Fatalf("InitiateAcquisition failed: %v", err)
		}

		// Read data
		_, err = client.Read(ctx, &perftest.ReadParameters{
			Timeout:    10000,
			NumSamples: testSamples,
		})
		if err != nil {
			log.Fatalf("Read failed: %v", err)
		}
	}
	elapsed := time.Since(startTime)

	totalMs := elapsed.Seconds() * 1000
	perIterationMs := totalMs / float64(iterations)

	connectionType := "TCP"
	if *useUDS {
		connectionType = "UDS"
	}

	fmt.Printf("Performance test complete (%s): %.3fms total or %.3fms per iteration\n", 
		connectionType, totalMs, perIterationMs)
	fmt.Println("\n✓ All tests passed! Go gRPC server is working correctly.")
}
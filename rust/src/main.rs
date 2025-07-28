use anyhow::Result;
use clap::Parser;
use std::net::SocketAddr;
use tonic::{transport::Server, Request, Response, Status};
use futures_util::stream;

pub mod ni_perf_test {
    tonic::include_proto!("ni_perf_test");
}

pub mod ni {
    pub mod data_monikers {
        tonic::include_proto!("ni.data_monikers");
    }
}

use ni_perf_test::ni_perf_test_service_server::{NiPerfTestService, NiPerfTestServiceServer};
use ni_perf_test::*;

use ni::data_monikers::moniker_service_server::{MonikerService, MonikerServiceServer};
use ni::data_monikers::*;

#[derive(Parser, Debug)]
#[command(name = "perftest_server")]
#[command(about = "gRPC server for testing various aspects of gRPC performance")]
struct Args {
    /// Path to the certificate file to be used
    #[arg(short, long, default_value = "")]
    cert: String,

    /// Port to listen on
    #[arg(short, long, default_value = "50051")]
    port: u16,

    /// Address to bind to
    #[arg(short, long, default_value = "0.0.0.0")]
    address: String,
}

#[derive(Debug, Default)]
pub struct NiPerfTestServer;

#[tonic::async_trait]
impl NiPerfTestService for NiPerfTestServer {
    type StreamLatencyTestStream = std::pin::Pin<Box<dyn futures_util::Stream<Item = Result<StreamLatencyServer, Status>> + Send>>;
    type StreamLatencyTestServerStream = std::pin::Pin<Box<dyn futures_util::Stream<Item = Result<StreamLatencyServer, Status>> + Send>>;
    type TestWriteContinuouslyStream = std::pin::Pin<Box<dyn futures_util::Stream<Item = Result<TestWriteResult, Status>> + Send>>;
    type TestSidebandStreamStream = std::pin::Pin<Box<dyn futures_util::Stream<Item = Result<TestSidebandStreamResponse, Status>> + Send>>;
    type ReadContinuouslyStream = std::pin::Pin<Box<dyn futures_util::Stream<Item = Result<ReadContinuouslyResult, Status>> + Send>>;

    async fn stream_latency_test(
        &self,
        request: Request<tonic::Streaming<StreamLatencyClient>>,
    ) -> Result<Response<Self::StreamLatencyTestStream>, Status> {
        let mut stream = request.into_inner();
        
        let output_stream = async_stream::stream! {
            loop {
                match stream.message().await {
                    Ok(Some(client_msg)) => {
                        let server_msg = StreamLatencyServer {
                            message: client_msg.message, // Echo back the client message
                        };
                        yield Ok(server_msg);
                    }
                    Ok(None) => {
                        // Stream ended
                        break;
                    }
                    Err(e) => {
                        yield Err(e);
                        break;
                    }
                }
            }
        };

        Ok(Response::new(Box::pin(output_stream)))
    }

    async fn stream_latency_test_client(
        &self,
        request: Request<tonic::Streaming<StreamLatencyClient>>,
    ) -> Result<Response<StreamLatencyServer>, Status> {
        let mut stream = request.into_inner();
        let mut last_message = 0;

        // Read all client messages and keep track of the last one
        while let Some(client_msg) = stream.message().await? {
            last_message = client_msg.message;
        }

        // Return the last message received
        Ok(Response::new(StreamLatencyServer {
            message: last_message,
        }))
    }

    async fn stream_latency_test_server(
        &self,
        request: Request<StreamLatencyClient>,
    ) -> Result<Response<Self::StreamLatencyTestServerStream>, Status> {
        let client_msg = request.into_inner();
        let responses = vec![Ok(StreamLatencyServer {
            message: client_msg.message,
        })];
        
        Ok(Response::new(Box::pin(stream::iter(responses.into_iter()))))
    }

    async fn test_write(
        &self,
        _request: Request<TestWriteParameters>,
    ) -> Result<Response<TestWriteResult>, Status> {
        Ok(Response::new(TestWriteResult { status: 0 }))
    }

    async fn test_write_continuously(
        &self,
        request: Request<tonic::Streaming<TestWriteParameters>>,
    ) -> Result<Response<Self::TestWriteContinuouslyStream>, Status> {
        let mut stream = request.into_inner();
        let mut responses = Vec::new();

        while let Some(_write_params) = stream.message().await? {
            responses.push(Ok(TestWriteResult { status: 0 }));
        }

        Ok(Response::new(Box::pin(stream::iter(responses.into_iter()))))
    }

    async fn begin_test_sideband_stream(
        &self,
        request: Request<BeginTestSidebandStreamRequest>,
    ) -> Result<Response<BeginTestSidebandStreamResponse>, Status> {
        let req = request.into_inner();
        
        Ok(Response::new(BeginTestSidebandStreamResponse {
            strategy: req.strategy,
            connection_url: "localhost:50052".to_string(),
            sideband_identifier: "rust_sideband_001".to_string(),
        }))
    }

    async fn test_sideband_stream(
        &self,
        request: Request<tonic::Streaming<TestSidebandStreamRequest>>,
    ) -> Result<Response<Self::TestSidebandStreamStream>, Status> {
        let mut stream = request.into_inner();
        let mut responses = Vec::new();

        while let Some(_sideband_req) = stream.message().await? {
            responses.push(Ok(TestSidebandStreamResponse {
                sideband_location: "localhost:50052".to_string(),
            }));
        }

        Ok(Response::new(Box::pin(stream::iter(responses.into_iter()))))
    }

    async fn init(
        &self,
        request: Request<InitParameters>,
    ) -> Result<Response<InitResult>, Status> {
        let req = request.into_inner();
        Ok(Response::new(InitResult { status: req.id }))
    }

    async fn configure_vertical(
        &self,
        _request: Request<ConfigureVerticalRequest>,
    ) -> Result<Response<ConfigureVerticalResponse>, Status> {
        Ok(Response::new(ConfigureVerticalResponse { status: 0 }))
    }

    async fn configure_horizontal_timing(
        &self,
        _request: Request<ConfigureHorizontalTimingRequest>,
    ) -> Result<Response<ConfigureHorizontalTimingResponse>, Status> {
        Ok(Response::new(ConfigureHorizontalTimingResponse { status: 0 }))
    }

    async fn initiate_acquisition(
        &self,
        _request: Request<InitiateAcquisitionRequest>,
    ) -> Result<Response<InitiateAcquisitionResponse>, Status> {
        Ok(Response::new(InitiateAcquisitionResponse { status: 0 }))
    }

    async fn read(
        &self,
        request: Request<ReadParameters>,
    ) -> Result<Response<ReadResult>, Status> {
        let req = request.into_inner();
        let samples = vec![8.325793493; req.num_samples as usize];
        
        Ok(Response::new(ReadResult {
            status: 0,
            samples,
        }))
    }

    async fn read_complex(
        &self,
        request: Request<ReadParameters>,
    ) -> Result<Response<ReadComplexResult>, Status> {
        let req = request.into_inner();
        let mut samples = Vec::new();
        
        for _ in 0..req.num_samples {
            samples.push(ComplexNumber {
                real: 3.14,
                imaginary: 4.56,
            });
        }
        
        Ok(Response::new(ReadComplexResult {
            status: 0,
            samples,
        }))
    }

    async fn read_complex_arena(
        &self,
        request: Request<ReadParameters>,
    ) -> Result<Response<ReadComplexResult>, Status> {
        // Same implementation as read_complex for simplicity
        self.read_complex(request).await
    }

    async fn read_continuously(
        &self,
        request: Request<ReadContinuouslyParameters>,
    ) -> Result<Response<Self::ReadContinuouslyStream>, Status> {
        let req = request.into_inner();
        let wfm = vec![0.0; req.num_samples as usize];
        let iterations = if req.num_iterations == 0 { 10000 } else { req.num_iterations };
        
        let mut responses = Vec::new();
        for _ in 0..iterations {
            responses.push(Ok(ReadContinuouslyResult {
                status: 0,
                wfm: wfm.clone(),
            }));
        }
        
        Ok(Response::new(Box::pin(stream::iter(responses.into_iter()))))
    }
}

#[derive(Debug, Default)]
pub struct NiMonikerServer;

#[tonic::async_trait]
impl MonikerService for NiMonikerServer {
    type StreamReadWriteStream = std::pin::Pin<Box<dyn futures_util::Stream<Item = Result<MonikerReadResult, Status>> + Send>>;
    type StreamReadStream = std::pin::Pin<Box<dyn futures_util::Stream<Item = Result<MonikerReadResult, Status>> + Send>>;
    type StreamWriteStream = std::pin::Pin<Box<dyn futures_util::Stream<Item = Result<StreamWriteResponse, Status>> + Send>>;

    async fn begin_sideband_stream(
        &self,
        request: Request<BeginMonikerSidebandStreamRequest>,
    ) -> Result<Response<BeginMonikerSidebandStreamResponse>, Status> {
        let req = request.into_inner();
        
        Ok(Response::new(BeginMonikerSidebandStreamResponse {
            strategy: req.strategy,
            connection_url: "localhost:50053".to_string(),
            sideband_identifier: "rust_moniker_001".to_string(),
            buffer_size: 1024 * 1024,
        }))
    }

    async fn stream_read_write(
        &self,
        request: Request<tonic::Streaming<MonikerWriteRequest>>,
    ) -> Result<Response<Self::StreamReadWriteStream>, Status> {
        let mut stream = request.into_inner();
        let mut responses = Vec::new();

        while let Some(_write_req) = stream.message().await? {
            responses.push(Ok(MonikerReadResult {
                data: None,
            }));
        }

        Ok(Response::new(Box::pin(stream::iter(responses.into_iter()))))
    }

    async fn stream_read(
        &self,
        _request: Request<MonikerList>,
    ) -> Result<Response<Self::StreamReadStream>, Status> {
        let responses = vec![Ok(MonikerReadResult {
            data: None,
        })];
        
        Ok(Response::new(Box::pin(stream::iter(responses.into_iter()))))
    }

    async fn stream_write(
        &self,
        request: Request<tonic::Streaming<MonikerWriteRequest>>,
    ) -> Result<Response<Self::StreamWriteStream>, Status> {
        let mut stream = request.into_inner();
        let mut responses = Vec::new();

        while let Some(_write_req) = stream.message().await? {
            responses.push(Ok(StreamWriteResponse {}));
        }

        Ok(Response::new(Box::pin(stream::iter(responses.into_iter()))))
    }
}

#[tokio::main]
async fn main() -> Result<()> {
    let args = Args::parse();
    
    let addr: SocketAddr = format!("{}:{}", args.address, args.port).parse()?;
    
    let perf_service = NiPerfTestServiceServer::new(NiPerfTestServer::default());
    let moniker_service = MonikerServiceServer::new(NiMonikerServer::default());
    
    println!("Starting Rust gRPC server on {}", addr);
    
    Server::builder()
        .add_service(perf_service)
        .add_service(moniker_service)
        .serve(addr)
        .await?;
    
    Ok(())
}

import grpc
import time
import perftest_pb2 as perftest
import perftest_pb2_grpc as perftest_rpc
import SharedMemoryForwardingInterceptor


server_address = "localhost"
server_port = "50051"
init_id = 42
iterations = 1000


def GetElapsedTime(start_time_secs):
    stop_time_secs = time.perf_counter()
    return stop_time_secs - start_time_secs


def RunMessagePerformanceTestSuite(client):
    print("Start Message Performance Test Suite")
    start_time_secs = time.perf_counter()
    for x in range(iterations):
        client.Init(perftest.InitParameters(id=init_id))
    messages_per_second = iterations / GetElapsedTime(start_time_secs)
    print(f"Result: {messages_per_second:.1f} messages/s\n")


def RunClient():
    channel = grpc.insecure_channel(f"{server_address}:{server_port}")
    no_intercept_client = perftest_rpc.niPerfTestServiceStub(channel)
    shared_memory_interceptor = SharedMemoryForwardingInterceptor.SharedMemoryForwardingInterceptor(
        no_intercept_client
    )
    print(f"Connecting to server at {server_address}:{server_port}")
    intercept_channel = grpc.intercept_channel(channel, shared_memory_interceptor)
    client = perftest_rpc.niPerfTestServiceStub(intercept_channel)
    init_result = client.Init(perftest.InitParameters(id=init_id))
    if init_result.status != init_id:
        print(f"Server communication failure! Got {str(init_result).strip()} != {init_id}")
        exit(1)
    else:
        print(f"Init result: {init_result}")
    RunMessagePerformanceTestSuite(client)


if __name__ == "__main__":
    RunClient()
    exit(0)

import grpc
import time
import perftest_pb2 as perftest
import perftest_pb2_grpc as perftest_rpc

server_address = "localhost"
server_port = "50051"

if __name__ == "__main__":
    channel = grpc.insecure_channel(f"{server_address}:{server_port}")
    client = perftest_rpc.niPerfTestServiceStub(channel)
    
    iterations = 1000
    samples = 10

    res = client.Init(perftest.InitParameters(command="", id=42))
    print(res)

    print( "Start Message Performance Test Suite")

    print("Start Messages per second test")

    start_time = time.perf_counter()

    for x in range(50000):
        client.Init(perftest.InitParameters(command="", id=42))

    stop_time = time.perf_counter()

    msgsPerSecond = (50000) / (stop_time - start_time)

    print(f"Result: {msgsPerSecond} messages per second")
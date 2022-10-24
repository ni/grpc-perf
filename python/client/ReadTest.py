import grpc
import time
import perftest_pb2 as perftest
import perftest_pb2_grpc as perftest_rpc

server_address = "localhost"
server_port = "50051"

if __name__ == "__main__":
    options = [('grpc.max_receive_message_length', 100 * 1024 * 1024)]
    channel = grpc.insecure_channel(f"{server_address}:{server_port}", options=options)
    client = perftest_rpc.niPerfTestServiceStub(channel)
    iterations = 1000
    samples = 800000
    complex_samples = 400000

    print("Connect")
    # Open session to NI-SCOPE module with options.
    client.Init(perftest.InitParameters(command="", id=0))
        
    print("Start Warmup")
    for x in range(10):
        client.Read(perftest.ReadParameters(timeout = 1000, num_samples = samples))

    print("Start Read Test: " + str(samples) + " samples")
    start_time = time.perf_counter()
    for x in range(iterations):
        client.Read(perftest.ReadParameters(timeout = 1000, num_samples = samples))

    stop_time = time.perf_counter()
    total_seconds = (stop_time - start_time)
    total_time = (stop_time - start_time) * 1000
    bytesPerSecond = (8.0 * samples * iterations) / total_seconds
    MBPerSecond = bytesPerSecond / (1024.0 * 1024)
    print("TEST Complete: {:.3f}ms total or {:.3f}ms per iteration, {:.3f} MB/s".format(total_time,total_time/iterations,MBPerSecond))


    print("")
    print("Start Read Complex Test: " + str(complex_samples) + " samples")
    start_time = time.perf_counter()
    for x in range(iterations):
        client.ReadComplex(perftest.ReadParameters(timeout = 1000, num_samples = complex_samples))

    stop_time = time.perf_counter()
    total_seconds = (stop_time - start_time)
    total_time = (stop_time - start_time) * 1000
    bytesPerSecond = (16.0 * complex_samples * iterations) / total_seconds
    MBPerSecond = bytesPerSecond / (1024.0 * 1024)
    print("TEST Complete: {:.3f}ms total or {:.3f}ms per iteration, {:.3f} MB/s".format(total_time,total_time/iterations,MBPerSecond))

using System.Threading.Tasks;
using Grpc.Net.Client;

Console.WriteLine("Hello, World!");

using var channel = GrpcChannel.ForAddress("http://localhost:50051");
var client = new NiPerfTest.niPerfTestService.niPerfTestServiceClient(channel);
var initResult = await client.InitAsync(new NiPerfTest.InitParameters { Id = 42 });
Console.WriteLine(initResult.Status);

var timer = new System.Diagnostics.Stopwatch();
timer.Start();

for (int x=0; x<10000; ++x)
{
    var initResult2 = await client.InitAsync(new NiPerfTest.InitParameters { Id = 42 });
}

timer.Stop();
Console.WriteLine($"Total time (ms): {timer.ElapsedMilliseconds}, Average time (ms): {timer.ElapsedMilliseconds / 10000.0}");
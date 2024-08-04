using System.IO.Pipes;
using System.Net;
using System.Net.Sockets;
using System.Security.Principal;
using System.Threading.Tasks;
using Grpc.Net.Client;

Console.WriteLine("Hello, World!");

string SocketPath = Path.Combine(Path.GetTempPath(), "socket.tmp"); 

//var udsEndPoint = new UnixDomainSocketEndPoint(SocketPath);
//var connectionFactory = new UnixDomainSocketsConnectionFactory(udsEndPoint);
//var connectionFactory = new NamedPipesConnectionFactory("grpc-perf");
//var socketsHttpHandler = new SocketsHttpHandler
//{
//    ConnectCallback = connectionFactory.ConnectAsync
//}; 

//using var channel = GrpcChannel.ForAddress("http://localhost", new GrpcChannelOptions
//{
//    HttpHandler = socketsHttpHandler
//});


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
double msgsPerSecond = (10000 * 1000.0) / (double)timer.ElapsedMilliseconds;
Console.WriteLine($"Messages per second: {msgsPerSecond}");

timer.Restart();
for (int x = 0; x < 10000; ++x)
{
    var initResult2 = await client.InitAsync(new NiPerfTest.InitParameters { Id = 42 });
}
timer.Stop();
Console.WriteLine($"Total time (ms): {timer.ElapsedMilliseconds}, Average time (ms): {timer.ElapsedMilliseconds / 10000.0}");
msgsPerSecond = (10000 * 1000.0) / (double)timer.ElapsedMilliseconds;
Console.WriteLine($"Messages per second: {msgsPerSecond}");


timer.Restart();
List<Task> tasks = new List<Task>(100);
for (int x = 0; x < 10000; ++x)
{
    tasks.Clear();
    for (int y = 0; y < 100; ++y)
    {
        var awaiter = client.InitAsync(new NiPerfTest.InitParameters { Id = 42 });
        tasks.Add(awaiter.ResponseAsync);        
    }
    await Task.WhenAll(tasks);
}
timer.Stop();
Console.WriteLine($"Async Total time (ms): {timer.ElapsedMilliseconds}, Average time (ms): {timer.ElapsedMilliseconds / (10000.0 * 100.0)}");
msgsPerSecond = (10000 * 1000.0 * 100.0) / (double)timer.ElapsedMilliseconds;
Console.WriteLine($"Messages per second: {msgsPerSecond}");

public class UnixDomainSocketsConnectionFactory
{
    private readonly EndPoint endPoint;

    public UnixDomainSocketsConnectionFactory(EndPoint endPoint)
    {
        this.endPoint = endPoint;
    }

    public async ValueTask<Stream> ConnectAsync(SocketsHttpConnectionContext _,
        CancellationToken cancellationToken = default)
    {
        var socket = new Socket(AddressFamily.Unix, SocketType.Stream, ProtocolType.Unspecified);

        try
        {
            await socket.ConnectAsync(this.endPoint, cancellationToken).ConfigureAwait(false);
            return new NetworkStream(socket, true);
        }
        catch
        {
            socket.Dispose();
            throw;
        }
    }
}

public class NamedPipesConnectionFactory
{
    private readonly string pipeName;

    public NamedPipesConnectionFactory(string pipeName)
    {
        this.pipeName = pipeName;
    }

    public async ValueTask<Stream> ConnectAsync(SocketsHttpConnectionContext _,
        CancellationToken cancellationToken = default)
    {
        var clientStream = new NamedPipeClientStream(
            serverName: ".",
            pipeName: this.pipeName,
            direction: PipeDirection.InOut,
            options: PipeOptions.WriteThrough | PipeOptions.Asynchronous,
            impersonationLevel: TokenImpersonationLevel.Anonymous);

        try
        {
            await clientStream.ConnectAsync(cancellationToken).ConfigureAwait(false);
            return clientStream;
        }
        catch
        {
            clientStream.Dispose();
            throw;
        }
    }
}

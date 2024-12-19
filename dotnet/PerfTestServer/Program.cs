using Grpc.Net.Client;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Server.Kestrel.Core;
using Microsoft.AspNetCore.TestHost;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace PerfTestServer
{
    public class Program
    {
        public static async Task Main(string[] args)
        {
            System.Console.WriteLine("Starting Server");
            var host = CreateHostBuilder(args).Build();
            
            await host.StartAsync();
            //var client = host.GetTestClient();
            TestServer testServer = host.GetTestServer();
            var handler = testServer.CreateHandler();

            using var channel = GrpcChannel.ForAddress("http://localhost", new GrpcChannelOptions
            {
                HttpHandler = handler
            });

            var client = new NiPerfTest.niPerfTestService.niPerfTestServiceClient(channel);
            var initResult = await client.InitAsync(new NiPerfTest.InitParameters { Id = 42 });
            Console.WriteLine(initResult.Status);

            var iterations = 50000;
            while (true)
            {
                var timer = new System.Diagnostics.Stopwatch();
                timer.Start();
                for (int x = 0; x < iterations; ++x)
                {
                    var initResult2 = await client.InitAsync(new NiPerfTest.InitParameters { Id = 42 });
                }
                timer.Stop();
                Console.WriteLine($"Total time (ms): {timer.ElapsedMilliseconds}, Average time (ms): {timer.ElapsedMilliseconds / (double)iterations}");
                double msgsPerSecond = (iterations * 1000.0) / (double)timer.ElapsedMilliseconds;
                Console.WriteLine($"Messages per second: {msgsPerSecond}");
                await Task.Delay(1000);
            }

        }

        public static IHostBuilder CreateHostBuilder(string[] args)
        {
            var socketPath = Path.Combine(Path.GetTempPath(), "socket.tmp");

            var builder = Host.CreateDefaultBuilder(args);
            builder.ConfigureLogging(config =>
                {
                    config.ClearProviders();
                });
            builder.ConfigureWebHostDefaults(webBuilder =>
                {
                    webBuilder.UseTestServer();
                    webBuilder.UseStartup<Startup>();
                    //webBuilder.UseUrls($"http://localhost:50051");
                    webBuilder.ConfigureKestrel(serverOptions =>
                    {
                        serverOptions.ListenAnyIP(50051, listenOptions =>
                        {
                            listenOptions.Protocols = HttpProtocols.Http2;
                        });
                        serverOptions.ListenUnixSocket(socketPath, listenOptions =>
                        {
                            listenOptions.Protocols = HttpProtocols.Http2;
                        });
                        serverOptions.ListenNamedPipe("grpc-perf", listenOptions =>
                        {
                            listenOptions.Protocols = HttpProtocols.Http2;
                        });
                    });
                });
            return builder;
        }
    }
}
   
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Server.Kestrel.Core;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using System.IO;

namespace PerfTestServer
{
    public class Program
    {
        public static void Main(string[] args)
        {
            System.Console.WriteLine("Starting Server");
            CreateHostBuilder(args).Build().Run();
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
   
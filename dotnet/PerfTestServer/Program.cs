using Microsoft.AspNetCore.Server.Kestrel.Core;
using System.IO.Compression;

Console.WriteLine("Starting .NET 9 gRPC Performance Server with Performance Optimizations");

// Set performance-oriented environment variables for synchronous completions
Environment.SetEnvironmentVariable("DOTNET_SYSTEM_NET_SOCKETS_INLINE_COMPLETIONS", "1");
Environment.SetEnvironmentVariable("DOTNET_SYSTEM_NET_HTTP_SOCKETSHTTPHANDLER_HTTP2SUPPORT", "true");
Environment.SetEnvironmentVariable("DOTNET_ThreadPool_ForceMinWorkerThreads", "100");
Environment.SetEnvironmentVariable("DOTNET_ThreadPool_ForceMaxWorkerThreads", "1000");
Environment.SetEnvironmentVariable("DOTNET_SYSTEM_GC_SERVER", "1");
Environment.SetEnvironmentVariable("DOTNET_gcServer", "1");
Environment.SetEnvironmentVariable("DOTNET_ReadyToRun", "1");
Console.WriteLine("Performance environment variables set");

var socketPath = Path.Combine(Path.GetTempPath(), "perftest");

// Clean up any existing socket file to prevent "address in use" errors
if (File.Exists(socketPath))
{
    Console.WriteLine($"Removing existing socket file: {socketPath}");
    File.Delete(socketPath);
}

var builder = WebApplication.CreateBuilder(args);

// Configure ThreadPool for better performance
ThreadPool.SetMinThreads(Environment.ProcessorCount * 4, Environment.ProcessorCount * 4);
ThreadPool.SetMaxThreads(Environment.ProcessorCount * 32, Environment.ProcessorCount * 32);

// Clear default logging providers for performance
builder.Logging.ClearProviders();

// Add gRPC services with enhanced performance settings
builder.Services.AddGrpc(options =>
{
    options.ResponseCompressionLevel = CompressionLevel.NoCompression;
    // Match C++ server settings for compatibility
    options.MaxReceiveMessageSize = 1024 * 1024; // 1MB like C++ server
    options.MaxSendMessageSize = 1024 * 1024;    // 1MB like C++ server
    options.EnableDetailedErrors = false;        // Disable for performance
    
    // Performance optimizations
    options.IgnoreUnknownServices = true;        // Skip unknown service lookup
}).AddServiceOptions<PerfTestServer.PerftestServiceImpl>(options =>
{
    // Service-specific performance options
    options.MaxReceiveMessageSize = 1024 * 1024;
    options.MaxSendMessageSize = 1024 * 1024;
});

// Configure Kestrel for multiple transports with performance optimizations
builder.WebHost.ConfigureKestrel(serverOptions =>
{
    // Performance optimizations for synchronous processing
    serverOptions.AllowSynchronousIO = true;
    serverOptions.AllowAlternateSchemes = true;
    
    // HTTP/2 on TCP with performance settings
    serverOptions.ListenAnyIP(50051, listenOptions =>
    {
        listenOptions.Protocols = HttpProtocols.Http2;
    });
    
    // Unix Domain Socket (Linux/macOS)  
    // Use HTTP/2 only - same as TCP endpoint for consistency
    if (!OperatingSystem.IsWindows())
    {
        serverOptions.ListenUnixSocket(socketPath, listenOptions =>
        {
            // Use HTTP/2 only, same as TCP endpoint
            listenOptions.Protocols = HttpProtocols.Http2;
        });
    }
    
    // HTTP/2 on Named Pipe (Windows only)
    if (OperatingSystem.IsWindows())
    {
        serverOptions.ListenNamedPipe("grpc-perf", listenOptions =>
        {
            listenOptions.Protocols = HttpProtocols.Http2;
        });
    }
});

var app = builder.Build();

// Configure the HTTP request pipeline
if (app.Environment.IsDevelopment())
{
    app.UseDeveloperExceptionPage();
}

app.UseRouting();

// Map gRPC service
app.MapGrpcService<PerfTestServer.PerftestServiceImpl>();

// Default response for non-gRPC requests
app.MapGet("/", () => "Communication with gRPC endpoints must be made through a gRPC client.");

Console.WriteLine($"gRPC Server listening on:");
Console.WriteLine($"  TCP: http://0.0.0.0:50051");
if (!OperatingSystem.IsWindows())
{
    Console.WriteLine($"  Unix Socket: {socketPath}");
}
if (OperatingSystem.IsWindows())
{
    Console.WriteLine($"  Named Pipe: grpc-perf");
}

// Setup graceful shutdown to clean up socket file
var cancellationToken = new CancellationTokenSource();
Console.CancelKeyPress += (sender, e) =>
{
    e.Cancel = true;
    Console.WriteLine("\nShutting down gracefully...");
    cancellationToken.Cancel();
};

AppDomain.CurrentDomain.ProcessExit += (sender, e) =>
{
    if (File.Exists(socketPath))
    {
        Console.WriteLine($"Cleaning up socket file: {socketPath}");
        File.Delete(socketPath);
    }
};

try
{
    await app.RunAsync(cancellationToken.Token);
}
finally
{
    if (File.Exists(socketPath))
    {
        Console.WriteLine($"Cleaning up socket file: {socketPath}");
        File.Delete(socketPath);
    }
}
   
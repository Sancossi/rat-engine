using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using ModelContextProtocol.Server;
using Rat.StrideMcp.Server;
using Rat.StrideMcp;

if (args.Length != 2 || args[0] != "--connection")
{ Console.Error.WriteLine("Usage: Rat.StrideMcp.Server --connection <absolute descriptor.json>"); return 2; }
var bridge = new BridgeClient(args[1]);
var builder = Host.CreateApplicationBuilder();
builder.Logging.ClearProviders();
builder.Logging.AddConsole(options => options.LogToStandardErrorThreshold = LogLevel.Trace);
builder.Services.AddSingleton(bridge);
builder.Services.AddMcpServer().WithStreamServerTransport(new LineLimitStream(Console.OpenStandardInput(),65536),Console.OpenStandardOutput()).WithToolsFromAssembly();
await builder.Build().RunAsync();
return 0;

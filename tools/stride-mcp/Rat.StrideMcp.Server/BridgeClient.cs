using System.IO.Pipes;
using System.Runtime.InteropServices;
using System.Text.Json.Nodes;
using Microsoft.Win32.SafeHandles;
using ModelContextProtocol.Protocol;

namespace Rat.StrideMcp.Server;

public sealed class BridgeClient
{
    private readonly JsonObject descriptor;
    private readonly SemaphoreSlim gate = new(1);
    [DllImport("kernel32.dll", SetLastError=true)]
    private static extern bool GetNamedPipeServerProcessId(SafePipeHandle handle, out uint processId);
    public BridgeClient(string path)
    {
        if (!Path.IsPathFullyQualified(path)) throw new ArgumentException("Connection path must be absolute.");
        descriptor = JsonNode.Parse(File.ReadAllText(path))!.AsObject();
        if (descriptor["protocol"]?.GetValue<string>() != "rat-editor-pipe/1" || !descriptor["pipeName"]!.GetValue<string>().StartsWith("rat-stride-mcp-",StringComparison.Ordinal))
            throw new InvalidDataException("Unsupported connection descriptor.");
    }
    public async Task<CallToolResult> Call(string command, object arguments, CancellationToken cancellation)
    {
        using var deadline = CancellationTokenSource.CreateLinkedTokenSource(cancellation);
        deadline.CancelAfter(12000);
        await gate.WaitAsync(deadline.Token);
        try
        {
            using var pipe = new NamedPipeClientStream(".",descriptor["pipeName"]!.GetValue<string>(),PipeDirection.InOut,PipeOptions.Asynchronous|PipeOptions.CurrentUserOnly);
            await pipe.ConnectAsync(3000,deadline.Token);
            if (!GetNamedPipeServerProcessId(pipe.SafePipeHandle,out uint pid) || pid != descriptor["processId"]!.GetValue<int>())
                throw new InvalidOperationException("Named pipe belongs to a different editor process.");
            var request = Wire.Object(new {command,arguments,timeoutMs=10000,address=new {processId=(int)pid,
                projectId=descriptor["projectId"]!.GetValue<string>(),sessionId=descriptor["sessionId"]!.GetValue<string>()}});
            await Wire.Write(pipe,request,Wire.MaximumRequest,deadline.Token);
            var result = await Wire.Read(pipe,Wire.MaximumResponse,deadline.Token);
            var content = new List<ContentBlock>();
            if(result["data"] is JsonObject data && data.Remove("pngBase64",out var png))
                content.Add(new ImageContentBlock {MimeType="image/png",Data=System.Text.Encoding.UTF8.GetBytes(png!.GetValue<string>())});
            content.Insert(0,new TextContentBlock {Text=result.ToJsonString()});
            return new CallToolResult { Content=content, IsError=result["ok"]?.GetValue<bool>()!=true };
        }
        catch(Exception error) when(error is not OperationCanceledException || !cancellation.IsCancellationRequested)
        { return new CallToolResult { IsError=true, Content=[new TextContentBlock {Text=Wire.Object(new {ok=false,error=error.GetBaseException().Message}).ToJsonString()}] }; }
        finally { gate.Release(); }
    }
}

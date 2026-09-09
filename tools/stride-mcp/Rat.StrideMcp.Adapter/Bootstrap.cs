using System.IO;
using System.IO.Pipes;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json.Nodes;
using System.Windows.Threading;
using Stride.Core.Assets.Editor.Services;
using Stride.Core.Assets.Editor.ViewModel;
using Stride.Core.Diagnostics;

namespace Rat.StrideMcp.Adapter;

public static class Bootstrap
{
    internal static string ConnectionPath="",PipeName="";
    public static void Register(string connectionPath,string pipeName)
    {
        ConnectionPath=connectionPath;PipeName=pipeName;
        AssetsPlugin.RegisterPlugin(typeof(EditorPlugin));
    }
}
public sealed class EditorPlugin : AssetsPlugin
{
    private CancellationTokenSource? lifetime;
    public override void InitializePlugin(ILogger logger) { }
    public override void RegisterPrimitiveTypes(ICollection<Type> primitiveTypes) { }
    public override void InitializeSession(SessionViewModel session)
    {
        lifetime?.Cancel();lifetime=new();
        var dispatcher=Dispatcher.CurrentDispatcher;
        var bridge=new EditorBridge(session,dispatcher);
        var file=Wire.Object(new {pipeName=Bootstrap.PipeName,processId=Environment.ProcessId,projectId=bridge.ProjectId,sessionId=bridge.SessionId,
            projectPath=session.SessionFilePath.ToString(),protocol="rat-editor-pipe/1",stride="4.4.0-dev",startedUtc=DateTime.UtcNow});
        File.WriteAllText(Bootstrap.ConnectionPath,file.ToJsonString());
        var token=lifetime.Token;
        _=Task.Run(()=>Listen(bridge,token));
    }
    protected override void SessionDisposed(SessionViewModel session){lifetime?.Cancel();}
    private static async Task Listen(EditorBridge bridge,CancellationToken lifetime)
    {
        while(!lifetime.IsCancellationRequested)
        {
            try
            {
                using var pipe=new NamedPipeServerStream(Bootstrap.PipeName,PipeDirection.InOut,1,PipeTransmissionMode.Byte,PipeOptions.Asynchronous|PipeOptions.CurrentUserOnly);
                await pipe.WaitForConnectionAsync(lifetime);
                using var requestTimeout=CancellationTokenSource.CreateLinkedTokenSource(lifetime);requestTimeout.CancelAfter(10000);
                var request=await Wire.Read(pipe,Wire.MaximumRequest,requestTimeout.Token);
                int timeout=request["timeoutMs"]?.GetValue<int>()??5000;
                if(timeout<50||timeout>10000)throw new InvalidDataException("timeoutMs must be 50..10000.");
                using var cancellation=CancellationTokenSource.CreateLinkedTokenSource(lifetime);cancellation.CancelAfter(timeout);
                // EOF/cancellation on the one-request connection aborts queued UI work.
                var cancelled=WatchCancellation(pipe,cancellation);
                JsonObject response;
                try {response=await bridge.Dispatch(request,cancellation.Token);}
                catch(Exception error){response=Wire.Object(new {ok=false,error=error is OperationCanceledException?"Request cancelled before execution or capture completed.":error.GetBaseException().Message});}
                using var responseTimeout=CancellationTokenSource.CreateLinkedTokenSource(lifetime);
                responseTimeout.CancelAfter(10000);
                try {await Wire.Write(pipe,response,Wire.MaximumResponse,responseTimeout.Token);}
                finally {cancellation.Cancel();await cancelled;}
            }
            catch(OperationCanceledException) when(lifetime.IsCancellationRequested){break;}
            catch(Exception error){File.AppendAllText(Bootstrap.ConnectionPath+".adapter.log",DateTime.UtcNow+" "+error+Environment.NewLine);}
        }
    }
    private static async Task WatchCancellation(Stream pipe,CancellationTokenSource cancellation)
    {
        try {await Wire.Read(pipe,128,cancellation.Token);cancellation.Cancel();}
        catch {try{cancellation.Cancel();}catch(ObjectDisposedException){}}
    }
}

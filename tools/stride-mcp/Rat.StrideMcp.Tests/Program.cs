using System.IO;
using System.Text.Json.Nodes;
using System.Windows.Threading;
using Rat.StrideMcp;

var ready=new TaskCompletionSource<Dispatcher>(TaskCreationOptions.RunContinuationsAsynchronously);
var thread=new Thread(()=>{ready.SetResult(Dispatcher.CurrentDispatcher);Dispatcher.Run();});
thread.SetApartmentState(ApartmentState.STA);thread.Start();
var dispatcher=await ready.Task;
int mutations=0;
try
{
    using var entered=new ManualResetEventSlim();
    using var release=new ManualResetEventSlim();
    var blocker=dispatcher.InvokeAsync(()=>{entered.Set();if(!release.Wait(5000))throw new TimeoutException();});
    if(!entered.Wait(5000))throw new Exception("Dispatcher did not reach the blocking fixture.");
    using var deadline=new CancellationTokenSource(50);
    var pending=UiRequestQueue.Run(dispatcher,()=>Task.FromResult(++mutations),deadline.Token);
    await Task.Delay(150);
    release.Set();await blocker.Task;
    try{await pending;throw new Exception("Expired queued mutation unexpectedly completed.");}
    catch(OperationCanceledException){}
    await dispatcher.InvokeAsync(()=>{}).Task;
    if(mutations!=0)throw new Exception("Late mutation happened after deadline.");
    var value=await UiRequestQueue.Run(dispatcher,()=>Task.FromResult(++mutations),CancellationToken.None);
    if(value!=1)throw new Exception("A fresh valid request could not run after cancellation.");
    using var wire=new MemoryStream();
    var expected=new JsonObject{{"value","actual framed roundtrip"}};
    await Wire.Write(wire,expected,1024,CancellationToken.None);wire.Position=0;
    if((await Wire.Read(wire,1024,CancellationToken.None)).ToJsonString()!=expected.ToJsonString())throw new Exception("Framing changed payload.");
    foreach(var size in new[]{-1,0,65537})
    {
        using var invalid=new MemoryStream(BitConverter.GetBytes(size));
        try{await Wire.Read(invalid,65536,CancellationToken.None);throw new Exception("Invalid frame size accepted.");}
        catch(InvalidDataException){}
    }
    using var lines=new LineLimitStream(new MemoryStream("1234\n12345"u8.ToArray()),4);
    var chunk=new byte[3];
    try{while(await lines.ReadAsync(chunk)>0){}throw new Exception("Oversized SDK input was accepted.");}
    catch(InvalidDataException){}
    Console.WriteLine("PASS: queued cancellation causes no late mutation; next request runs; framing roundtrip, 3 invalid lengths and oversized SDK input rejected.");
}
finally{dispatcher.BeginInvokeShutdown(DispatcherPriority.Send);thread.Join(5000);}

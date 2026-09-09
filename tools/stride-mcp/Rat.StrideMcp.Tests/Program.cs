using System.IO;
using System.Text.Json.Nodes;
using System.Windows.Threading;
using Rat.StrideMcp;
using Rat.StrideMcp.Adapter;

// A real directory replacement must not turn an inspected local asset into
// an editable external asset on the next command.
var pathFixture=Path.Combine(Path.GetTempPath(),"rat-mcp-path-"+Guid.NewGuid().ToString("N"));
var project=Path.Combine(pathFixture,"project");var outside=Path.Combine(pathFixture,"project-other");
var assets=Path.Combine(project,"Assets");
Directory.CreateDirectory(assets);Directory.CreateDirectory(outside);
File.WriteAllText(Path.Combine(outside,"sample.sdtex"),"owned path fixture");
bool linked=false;
try
{
    if(!AssetPaths.Inside(project,Path.Combine(assets,"new.sdtex"))||AssetPaths.Inside(project,Path.Combine(outside,"sample.sdtex"))||AssetPaths.Inside(project,Path.Combine(project,"..","project-other","sample.sdtex")))throw new Exception("Project path boundary failed.");
    Directory.Delete(assets);
    // Junction creation does not require the symlink privilege on Windows.
    var linkStart=new System.Diagnostics.ProcessStartInfo("cmd.exe",$"/d /c mklink /J \"{assets}\" \"{outside}\""){UseShellExecute=false,CreateNoWindow=true,RedirectStandardOutput=true,RedirectStandardError=true};
    using(var linkProcess=System.Diagnostics.Process.Start(linkStart)??throw new Exception("Cannot start junction fixture"))
    {
        if(!linkProcess.WaitForExit(5000)||linkProcess.ExitCode!=0)throw new Exception("Cannot create owned junction fixture: "+linkProcess.StandardError.ReadToEnd());
        linked=true;
    }
    if(AssetPaths.Inside(project,Path.Combine(assets,"sample.sdtex")))throw new Exception("Post-inspection reparse replacement escaped the project root.");
}
finally
{
    if(linked)Directory.Delete(assets);else if(Directory.Exists(assets))Directory.Delete(assets);
    File.Delete(Path.Combine(outside,"sample.sdtex"));Directory.Delete(outside);Directory.Delete(project);Directory.Delete(pathFixture);
}
Console.WriteLine("PASS: sibling/traversal paths and actual post-inspection directory junction replacement rejected.");

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

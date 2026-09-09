using System.ComponentModel;
using System.IO;
using System.Reflection;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Windows.Threading;
using Rat.StrideMcp.Adapter;
using Stride.Assets.Entities;
using Stride.Core.Assets.Editor.Services;
using Stride.Core.Assets.Editor.ViewModel;
using Stride.Core.Diagnostics;

namespace Rat.StrideMcp.Qualification;

// This assembly is loaded only by verify-session-state.ps1. No test/eval tools
// are added to MCP. The fixture invokes the actual native SessionViewModel APIs.
public sealed class QualificationPlugin:AssetsPlugin
{
    public override void InitializePlugin(ILogger logger){}
    public override void RegisterPrimitiveTypes(ICollection<Type> primitiveTypes){}
    public override void InitializeSession(SessionViewModel session){_=StartupHook.Mode switch{"resources"=>ResourceQualification.Run(session),"creation-failure" or "creation-reopen"=>CreationFailureQualification.Run(session),_=>Run(session)};}
    private static async Task Run(SessionViewModel session)
    {
        var evidence=new Dictionary<string,object>();
        try
        {
            await Task.Delay(1500);
            var dispatcher=Dispatcher.CurrentDispatcher;
            var bridge=new EditorBridge(session,dispatcher);
            var asset=session.AllAssets.Single(a=>a.Asset is SceneAsset&&a.Id.ToString()=="58182787-af94-441d-94c8-e94ca4d17b57");
            var entity=((SceneAsset)asset.Asset).Hierarchy.Parts.Values.Single(p=>p.Entity.Name=="Qualification box").Entity;
            var identity=entity.Components.Single(c=>c.GetType().FullName=="Rat.Expedition.Authoring.ExpeditionIdentityComponent");
            var label=identity.GetType().GetProperty("DisplayLabel")!;
            string original=(string)label.GetValue(identity)!;
            string baseline=original+" [native save probe]";
            string disk=asset.AssetItem.FullPath.ToString();
            JsonObject Request(string command,object arguments)=>new(){["command"]=command,["arguments"]=JsonSerializer.SerializeToNode(arguments),
                ["address"]=new JsonObject{{"processId",Environment.ProcessId},{"projectId",bridge.ProjectId},{"sessionId",bridge.SessionId}}};
            async Task<long> Revision()
            {
                // Native property notifications are coalesced asynchronously.
                // Read after their refresh, as a real client does between edits.
                await Task.Delay(300);
                return (await bridge.Dispatch(Request("status",new{}),CancellationToken.None))["revision"]!.GetValue<long>();
            }
            JsonObject Change(string value,long revision)=>Request("set_property",new {sceneId=asset.Id.ToString(),entityId=entity.Id.ToString(),componentId=identity.Id.ToString(),property="DisplayLabel",value,expectedRevision=revision});
            async Task Set(string value)=>await bridge.Dispatch(Change(value,await Revision()),CancellationToken.None);
            async Task<string> Rejected(Task<JsonObject> task)
            {
                try{await task;throw new Exception("Guard accepted a forbidden mutation.");}
                catch(InvalidOperationException error) when(error.Message.Contains("Native")){return error.Message;}
            }
            void Assert(bool condition,string message){if(!condition)throw new Exception(message);}
            await Set(baseline);
            long revision=await Revision();
            Task<JsonObject>? concurrent=null;
            PropertyChangedEventHandler? saving=null;
            saving=(_,change)=>{
                if(change.PropertyName!=nameof(SessionViewModel.IsSaving)||!session.IsSaving)return;
                session.PropertyChanged-=saving;
                // Queue before SaveSessionCore reaches Task.Run: even fast disk IO
                // cannot enqueue its continuation ahead of this request.
                concurrent=bridge.Dispatch(Change("MUST NOT REACH DISK",revision),CancellationToken.None);
            };
            session.PropertyChanged+=saving;
            var nativeSave=session.SaveSession(); // deliberately not MCP save_session
            Assert(concurrent is not null,"Native saving event did not enqueue the competing request.");
            evidence["nativeSaveConcurrentRequest"]=await Rejected(concurrent!).WaitAsync(TimeSpan.FromSeconds(10));
            Assert(await nativeSave,"Native save failed.");
            Assert(!session.IsSaving&&!asset.IsDirty,"Save state or dirty flag remained set.");
            Assert((string)label.GetValue(identity)! ==baseline,"Rejected mutation changed memory.");
            Assert(File.ReadAllText(disk).Contains(baseline),"Native save did not persist baseline.");
            Assert(!File.ReadAllText(disk).Contains("MUST NOT REACH DISK"),"Rejected mutation reached disk.");
            await Set("Valid edit after native save");
            Assert(asset.IsDirty,"Post-save edit was incorrectly marked clean.");
            Assert(!File.ReadAllText(disk).Contains("Valid edit after native save"),"Unsaved edit unexpectedly reached disk.");
            var state=(await bridge.Dispatch(Request("status",new{}),CancellationToken.None))["data"]!;
            await bridge.Dispatch(Request("undo",new {expectedRevision=await Revision(),expectedTransactionId=state["undoTransactionId"]!.GetValue<string>()}),CancellationToken.None);
            Assert((string)label.GetValue(identity)! ==baseline&&!asset.IsDirty,"Undo did not restore saved baseline and clean state.");
            evidence["diskDirtyUndoParity"]=true;

            // A failing native Save must release its state even before serialization.
            PropertyChangedEventHandler? failSave=null;
            failSave=(_,change)=>{
                if(change.PropertyName==nameof(SessionViewModel.IsSaving)&&session.IsSaving){session.PropertyChanged-=failSave;throw new InvalidOperationException("Fixture save failure");}
            };
            session.PropertyChanged+=failSave;
            try{await session.SaveSession();throw new Exception("Expected native save fixture failure.");}
            catch(InvalidOperationException error) when(error.Message=="Fixture save failure"){}
            Assert(!session.IsSaving,"Failed save leaked busy state.");
            evidence["failedSaveReleased"]=true;

            await Set(original); // Restore the committed fixture through Quantum.
            var originalDialogs=session.Dialogs;
            var proxy=DispatchProxy.Create<IEditorDialogService,DialogProxy>();
            var answers=(DialogProxy)(object)proxy;answers.Inner=originalDialogs;
            session.ServiceProvider.UnregisterService(originalDialogs);session.ServiceProvider.RegisterService(proxy);
            try
            {
                var answer=new TaskCompletionSource<int>(TaskCreationOptions.RunContinuationsAsynchronously);
                answers.Answer=answer.Task;
                var closing=session.Close();
                Assert(session.IsClosing,"Close prompt did not set closing state.");
                evidence["pendingCloseRejected"]=await Rejected(bridge.Dispatch(Change("CLOSE MUST NOT WRITE",revision),CancellationToken.None));
                answer.SetResult(3);
                Assert(!await closing&&!session.IsClosing,"Cancelled close remained terminal.");
                answers.Answer=Task.FromException<int>(new InvalidOperationException("Fixture close failure"));
                try{await session.Close();throw new Exception("Expected close failure.");}
                catch(InvalidOperationException error) when(error.Message=="Fixture close failure"){}
                Assert(!session.IsClosing,"Failed close leaked busy state.");
                evidence["cancelledAndFailedCloseReleased"]=true;
                bool nested=false;
                PropertyChangedEventHandler nesting=(_,change)=>{if(change.PropertyName==nameof(SessionViewModel.IsSaving)&&session.IsSaving)nested|=session.IsClosing;};
                session.PropertyChanged+=nesting;
                answers.Answer=Task.FromResult(1); // native Close -> SaveSession
                Assert(await session.Close(),"Native close/save failed.");
                session.PropertyChanged-=nesting;
                Assert(nested&&session.IsClosing&&!session.IsSaving,"Close/save nesting or accepted terminal state failed.");
                Assert(File.ReadAllText(disk).Contains(original)&&!asset.IsDirty,"Close/save did not persist restored fixture.");
                evidence["closeSaveNestedAndPersisted"]=true;
                evidence["acceptedCloseRejected"]=await Rejected(bridge.Dispatch(Change("AFTER CLOSE",revision),CancellationToken.None));
                try{await session.SaveSession();throw new Exception("Save after accepted close was allowed.");}
                catch(InvalidOperationException error) when(error.Message.Contains("closed or disposed")){}
            }
            finally{session.ServiceProvider.UnregisterService(proxy);session.ServiceProvider.RegisterService(originalDialogs);}

            // Reproduce the GameStudioWindow sequence: queue Normal request, close
            // native asset editors, Destroy the session, then let the queue drain.
            var queued=bridge.Dispatch(Change("AFTER DESTROY",revision),CancellationToken.None);
            session.ServiceProvider.Get<IAssetEditorsManager>().CloseAllEditorWindows(false);
            session.Destroy();
            Assert(session.IsSessionDisposed,"Disposal was not visible before queued callback.");
            evidence["queuedAfterDestroyRejected"]=await Rejected(queued);
            Assert((string)label.GetValue(identity)! ==original,"Closed/disposed callback changed the entity.");
            evidence["passed"]=true;
        }
        catch(Exception error){evidence["error"]=error.ToString();evidence["passed"]=false;}
        File.WriteAllText(StartupHook.ResultPath,JsonSerializer.Serialize(evidence,new JsonSerializerOptions{WriteIndented=true}));
        // The test runner closes exactly this owned process after reading evidence;
        // normal GameStudio close handlers must not destroy this session twice.
    }
}

public class DialogProxy:DispatchProxy
{
    public IEditorDialogService Inner=null!;
    public Task<int> Answer=Task.FromResult(3);
    public bool SuppressProgress;
    protected override object? Invoke(MethodInfo? targetMethod,object?[]? args)
    {
        if(SuppressProgress&&targetMethod!.Name=="ShowProgressWindow")return null;
        if(targetMethod!.Name=="MessageBoxAsync"&&targetMethod.ReturnType==typeof(Task<int>))return Answer;
        try{return targetMethod.Invoke(Inner,args);}
        catch(TargetInvocationException error) when(error.InnerException is not null){System.Runtime.ExceptionServices.ExceptionDispatchInfo.Capture(error.InnerException).Throw();throw;}
    }
}

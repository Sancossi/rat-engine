using System.Collections.Specialized;
using System.IO;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Windows.Threading;
using Rat.StrideMcp.Adapter;
using Stride.Assets.Materials;
using Stride.Assets.Textures;
using Stride.Core.Assets;
using Stride.Core.Assets.Editor.Services;
using Stride.Core.Assets.Editor.ViewModel;
using Stride.Core.Diagnostics;
using Stride.Core.Presentation.Services;
using Stride.Core.Transactions;

namespace Rat.StrideMcp.Qualification;

// Opt-in native fault qualification, never exposed as an MCP eval/tool.
internal static class CreationFailureQualification
{
    internal static async Task Run(SessionViewModel session)
    {
        var evidence=new Dictionary<string,object>();
        try
        {
            await Task.Delay(1500);
            var package=session.LocalPackages.Single(p=>p.Name=="Rat.Expedition.Authoring");
            var undo=session.ServiceProvider.Get<IUndoRedoService>();
            var bridge=new EditorBridge(session,Dispatcher.CurrentDispatcher);
            JsonObject Request(string command,object args)=>new(){["command"]=command,["arguments"]=JsonSerializer.SerializeToNode(args),["address"]=new JsonObject{{"processId",Environment.ProcessId},{"projectId",bridge.ProjectId},{"sessionId",bridge.SessionId}}};
            async Task<JsonObject> Status()=>await bridge.Dispatch(Request("status",new{}),CancellationToken.None);
            string folderPath=Path.Combine(package.RootDirectory.ToString(),"Assets","McpCreationFailure");
            void Assert(bool value,string message){if(!value)throw new InvalidOperationException(message);}
            if(StartupHook.Mode=="creation-reopen")
            {
                var previous=JsonNode.Parse(File.ReadAllText(Path.Combine(Path.GetDirectoryName(StartupHook.ResultPath)!,"result.json")))!;
                var reopened=session.GetAssetById(new AssetId(Guid.Parse(previous["successfulAssetId"]!.GetValue<string>())))??throw new InvalidOperationException("Saved native asset ID is absent after reopen.");
                Assert(reopened.Directory.Package==package,"Reopened asset belongs to another package.");
                Assert(reopened.Asset is TextureAsset texture&&texture.Width==73,"Saved native asset did not reopen with its property.");
                Assert(!session.AllAssets.Any(a=>a.IsDirty),"Reopened fixture is dirty.");
                evidence["reopenedAssetId"]=reopened.Id.ToString();
                evidence["reopened"]=true;
                var broken=undo.CreateTransaction();
                undo.PushOperation(new FailingRollback());
                try{undo.AbortTransaction(broken);throw new Exception("Rollback failure was swallowed.");}
                catch(InvalidOperationException error) when(error.Message=="Injected rollback failure"){}
                Assert(undo.HasFailedTransaction&&!await session.SaveSession(),"Native save accepted a failed rollback.");
                var poisoned=await Status();
                Assert(poisoned["data"]!["busy"]!.GetValue<bool>(),"Poisoned session advertised idle.");
                try
                {
                    await bridge.Dispatch(Request("asset_set",new{assetId=reopened.Id.ToString(),property="Width",value=74,expectedRevision=poisoned["revision"]!.GetValue<long>()}),CancellationToken.None);
                    throw new Exception("MCP mutated a poisoned session.");
                }
                catch(InvalidOperationException error) when(error.Message.Contains("rollback failed")){}
                Assert(((TextureAsset)reopened.Asset).Width==73,"Poisoned MCP request mutated the asset.");
                evidence["rollbackFailureRejectsNativeSaveAndMcpMutation"]=true;
            }
            else
            {
                Assert(!Directory.Exists(folderPath)&&!session.AllAssets.Any(a=>a.IsDirty),"Creation probe requires absent fixture folder and clean own session.");
                var folder=package.GetOrCreateAssetDirectory("McpCreationFailure",true);
                var baseline=package.CreateAssetsAtomic(folder,[new AssetItem("McpCreationFailure/Baseline",new TextureAsset())],new LoggerResult()).Single();
                Assert(await session.SaveSession(),"Cannot save own baseline.");
                using(var transaction=undo.CreateTransaction())session.AssetNodeContainer.GetOrCreateNode(baseline.Asset)["Width"].Update(75);
                undo.Undo();
                string History()=>string.Join("|",session.ActionHistory.Transactions.Select(t=>$"{t.Id}:{t.IsDone}"));
                string Dirty()=>string.Join("|",session.AllAssets.Where(a=>a.IsDirty).Select(a=>a.Id).OrderBy(x=>x.ToString()));
                string previousHistory=History(),previousDirty=Dirty();
                byte[] previousDisk=File.ReadAllBytes(baseline.AssetItem.FullPath.ToString());
                void Restored(IEnumerable<AssetItem> items)
                {
                    foreach(var item in items)
                    {
                        Assert(session.GetAssetById(item.Id)==null,"Failed asset is still session registered.");
                        Assert(session.GraphContainer.TryGetGraph(item.Id)==null,"Failed graph is still registered.");
                        Assert(!folder.Assets.Any(a=>a.Id==item.Id)&&!package.Package.Assets.Any(a=>a.Id==item.Id),"Failed asset remains in directory/package.");
                    }
                    Assert(History()==previousHistory&&undo.CanRedo,"Prior Undo/Redo changed or failed creation entered history.");
                    Assert(Dirty()==previousDirty,"Abort did not restore dirty snapshot.");
                    Assert(File.ReadAllBytes(baseline.AssetItem.FullPath.ToString()).SequenceEqual(previousDisk),"Failure changed saved baseline bytes.");
                    Assert(!undo.TransactionInProgress&&!undo.UndoRedoInProgress&&!undo.HasFailedTransaction,"Transaction remained busy/poisoned after successful rollback.");
                }
                for(int scenario=0;scenario<3;scenario++)
                {
                    long previousRevision=(await Status())["revision"]!.GetValue<long>();
                    var items=new[]{new AssetItem($"McpCreationFailure/First{scenario}",new MaterialAsset()),new AssetItem($"McpCreationFailure/Second{scenario}",new MaterialAsset())};
                    var logger=new LoggerResult();
                    NotifyCollectionChangedEventHandler fail=(_,change)=>{
                        var added=change.NewItems?.OfType<AssetViewModel>().FirstOrDefault();
                        if(scenario==0&&added?.Id==items[0].Id)logger.Error("Injected error after first native insertion.");
                        if(scenario!=0&&added?.Id==items[1].Id)throw new InvalidOperationException("Injected second constructor directory notification failure.");
                    };
                    EventHandler<TransactionEventArgs> failNotification=(_,_)=>throw new InvalidOperationException("Injected abort notification failure after rollback.");
                    ((INotifyCollectionChanged)folder.Assets).CollectionChanged+=fail;
                    if(scenario==2)undo.Aborted+=failNotification;
                    bool rejected=false;
                    try{package.CreateAssetsAtomic(folder,items,logger);}
                    catch(Exception error){rejected=true;evidence[$"failure{scenario}"]=error.ToString();}
                    finally{((INotifyCollectionChanged)folder.Assets).CollectionChanged-=fail;undo.Aborted-=failNotification;}
                    Assert(rejected,"Injected failure unexpectedly succeeded.");
                    Restored(items);
                    Assert((await Status())["revision"]!.GetValue<long>()>previousRevision,"Abort did not advance session revision.");
                    evidence[$"rollback{scenario}PreservedGraphDiskDirtyHistory"]=true;
                }
                var parent=undo.CreateTransaction();
                var completion=undo.TransactionCompletion;
                var child=undo.CreateTransaction();
                undo.AbortTransaction(child);
                Assert(!completion.IsCompleted&&undo.TransactionInProgress,"Child abort completed parent prematurely.");
                parent.Complete();
                Assert(completion.IsCompleted,"Parent completion was lost.");
                evidence["nestedParentCompletion"]=true;
                var success=package.CreateAssetsAtomic(folder,[new AssetItem("McpCreationFailure/Success",new TextureAsset{Width=73})],new LoggerResult()).Single();
                Assert(success.IsDirty,"Successful creation was not dirty.");
                undo.Undo();Assert(session.GetAssetById(success.Id)==null,"Successful creation Undo failed.");
                undo.Redo();Assert(session.GetAssetById(success.Id)==success,"Successful creation Redo failed.");
                Assert(await session.SaveSession()&&!success.IsDirty,"Subsequent successful creation failed to save.");
                Assert(File.Exists(success.AssetItem.FullPath.ToString()),"Native asset file not written.");
                evidence["successfulAssetId"]=success.Id.ToString();
                evidence["subsequentCreateUndoRedoSave"]=true;
            }
            evidence["processId"]=Environment.ProcessId;
            evidence["passed"]=true;
            session.ServiceProvider.Get<IAssetEditorsManager>().CloseAllEditorWindows(false);
            session.Destroy();
        }
        catch(Exception error){evidence["passed"]=false;evidence["error"]=error.ToString();}
        File.WriteAllText(StartupHook.ResultPath,JsonSerializer.Serialize(evidence,new JsonSerializerOptions{WriteIndented=true}));
    }
    private sealed class FailingRollback:Operation
    {
        protected override void Rollback()=>throw new InvalidOperationException("Injected rollback failure");
        protected override void Rollforward()=>throw new NotSupportedException();
    }
}

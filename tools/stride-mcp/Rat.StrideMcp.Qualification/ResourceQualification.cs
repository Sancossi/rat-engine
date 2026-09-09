using System.ComponentModel;
using System.IO;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Windows.Threading;
using Rat.StrideMcp.Adapter;
using Stride.Assets.Entities;
using Stride.Assets.Materials;
using Stride.Assets.Models;
using Stride.Assets.Sprite;
using Stride.Assets.SpriteFont;
using Stride.Assets.Textures;
using Stride.Assets.UI;
using Stride.Core.Assets;
using Stride.Core.Assets.Editor.Services;
using Stride.Core.Assets.Editor.ViewModel;
using Stride.Core.Assets.Tracking;
using Stride.Core.Diagnostics;
using Stride.Core.IO;
using Stride.Core.Mathematics;
using Stride.Core.Presentation.Services;
using Stride.Core.Storage;
using Stride.Engine;
using Stride.Graphics;
using Stride.Rendering;
using Stride.Rendering.Materials;
using Stride.Rendering.Materials.ComputeColors;
using Stride.UI.Controls;

namespace Rat.StrideMcp.Qualification;

// Opt-in fixture only. Backend delay/error injection is not exposed through MCP.
internal static class ResourceQualification
{
    internal static async Task Run(SessionViewModel session)
    {
        var result=new Dictionary<string,object>();
        try
        {
            await Task.Delay(1500);
            var undo=session.ServiceProvider.Get<IUndoRedoService>();
            var package=session.LocalPackages.Single(p=>p.Name=="Rat.Expedition.Authoring");
            string game=Path.GetDirectoryName(session.SessionFilePath.ToString())!;
            string sources=Path.Combine(package.RootDirectory.ToString(),"Resources","McpResourceQualification");
            string assets=Path.Combine(package.RootDirectory.ToString(),"Assets","McpResourceQualification");
            Require(!Directory.Exists(sources)&&!Directory.Exists(assets),"Fixture folders already exist; do not overwrite them.");
            Require(!session.AllAssets.Any(a=>a.IsDirty),"Own fixture session is not initially clean.");
            Directory.CreateDirectory(sources);
            string obj=Path.Combine(sources,"native-source-probe.obj");
            File.WriteAllText(obj,"v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 0 1\nf 1/1 2/2 3/3\n");
            string png=Path.Combine(sources,"rat.png");File.Copy(Path.Combine(game,"Content","sprites","rat.png"),png);
            string font=Path.Combine(sources,"NotoSans-Regular.ttf");File.Copy(Path.Combine(game,"Content","fonts","NotoSans-Regular.ttf"),font);
            var logger=new LoggerResult();
            var created=new Dictionary<string,AssetViewModel>();
            using(var transaction=undo.CreateTransaction())
            {
                var folder=package.GetOrCreateAssetDirectory("McpResourceQualification",true);
                AssetViewModel Add(string name,Asset asset)
                {
                    var vm=package.CreateAsset(folder,new AssetItem("McpResourceQualification/"+name,asset),true,logger);created.Add(name,vm);return vm;
                }
                Add("TextureA",new TextureAsset{Source=png,IsCompressed=false});
                Add("TextureB",new TextureAsset{Source=png,IsCompressed=false});
                var mat=Add("Material",new MaterialAsset{Attributes=new(){Diffuse=new MaterialDiffuseMapFeature(new ComputeColor(new Color4(.3f,.6f,.2f,1))),DiffuseModel=new MaterialDiffuseLambertModelFeature()}});
                Add("MaterialB",new MaterialAsset{Attributes=new(){Diffuse=new MaterialDiffuseMapFeature(new ComputeColor(new Color4(.6f,.3f,.2f,1))),DiffuseModel=new MaterialDiffuseLambertModelFeature()}});
                var model=new ModelAsset{Source=obj};model.Materials.Add(new ModelMaterial{Name="InitialMaterial",MaterialInstance=new(){Material=ContentReferenceHelper.CreateReference<Material>(mat)}});
                SourceHashesHelper.UpdateHashes(model,new Dictionary<UFile,ObjectId>{{obj,ObjectId.New()}});
                var modelVm=Add("Model",model);
                Add("ModelB",new ModelAsset{Source=obj});
                Add("Sheet",new SpriteSheetAsset{Sprites=[new(){Name="front",Source=png,TextureRegion=new(0,0,32,48),Center=new(16,43),CenterFromMiddle=false,PixelsPerUnit=60}]});
                var fontVm=Add("Font",new SpriteFontAsset{FontSource=new FileFontProvider{Source=font}});
                var page=new UIPageAsset();var text=new TextBlock{Name="Fixture text",Text="Native resource fixture",Font=ContentReferenceHelper.CreateReference<SpriteFont>(fontVm)};
                page.Hierarchy.RootParts.Add(text);page.Hierarchy.Parts.Add(new UIElementDesign(text));var pageVm=Add("Page",page);
                var scene=new SceneAsset();var entity=new Entity("Resource fixture"){new ModelComponent{Model=ContentReferenceHelper.CreateReference<Model>(modelVm)},new UIComponent{Page=ContentReferenceHelper.CreateReference<UIPage>(pageVm)}};
                scene.Hierarchy.RootParts.Add(entity);scene.Hierarchy.Parts.Add(new EntityDesign(entity));Add("Scene",scene);
                undo.SetName(transaction,"Create own MCP resource qualification fixtures");
            }
            Require(!logger.HasErrors,"Native fixture CreateAsset reported errors.");
            Require(await session.SaveSession(),"Could not save own native resource fixtures.");
            string ready=StartupHook.ResultPath+".ready.json";
            File.WriteAllText(ready,JsonSerializer.Serialize(new {assets=created.ToDictionary(p=>p.Key,p=>p.Value.Id.ToString()),sources,assetDirectory=assets,processId=Environment.ProcessId}));
            var clientDone=StartupHook.ResultPath+".client.json";
            for(int i=0;!File.Exists(clientDone)&&i<240;i++)await Task.Delay(250);
            Require(File.Exists(clientDone),"Official MCP client did not complete resource probes within 60 seconds.");
            var client=JsonNode.Parse(File.ReadAllText(clientDone))!;
            Require(client["passed"]!.GetValue<bool>(),"Official MCP resource probes failed.");
            result["mcpClient"]=client;
            Require(!session.AllAssets.Any(a=>a.IsDirty),"MCP client left unsaved changes.");

            PropertyChangedEventHandler? failLease=null;
            failLease=(_,e)=>{if(e.PropertyName==nameof(SessionViewModel.IsAssetOperationInProgress)&&session.IsAssetOperationInProgress){session.PropertyChanged-=failLease;throw new InvalidOperationException("fixture lease notification failure");}};
            session.PropertyChanged+=failLease;
            try{session.TryBeginAssetOperation(out _);throw new Exception("Expected lease acquisition exception.");}
            catch(InvalidOperationException e) when(e.Message=="fixture lease notification failure"){}
            Require(!session.IsAssetOperationInProgress,"Failed notification leaked reservation.");
            Require(session.TryBeginAssetOperation(out var lease),"Fresh lease failed after notification exception.");
            Require(!session.TryBeginAssetOperation(out _),"Nested lease was accepted.");lease.Dispose();lease.Dispose();
            result["leaseNotificationReentryAndDoubleDispose"]=true;

            var importer=new GatedModelImporter();AssetRegistry.RegisterImporter(importer);
            var vm=created["Model"];var definition=(ModelAsset)vm.Asset;
            string disk=vm.AssetItem.FullPath.ToString();string diskBefore=File.ReadAllText(disk);
            var bridge=new EditorBridge(session,Dispatcher.CurrentDispatcher);
            JsonObject Request(string cmd,object args)=>new(){["command"]=cmd,["arguments"]=JsonSerializer.SerializeToNode(args),["address"]=JsonSerializer.SerializeToNode(new{processId=Environment.ProcessId,projectId=bridge.ProjectId,sessionId=bridge.SessionId})};
            importer.Reset("success");var importLog=new LoggerResult();
            using(var transaction=undo.CreateTransaction())
            {
                var updating=vm.Sources.UpdateAssetFromSource(importLog);
                await importer.Entered.Task.WaitAsync(TimeSpan.FromSeconds(5));
                Require(session.IsAssetOperationInProgress,"Actual native reimport did not reserve session.");
                Require(!await session.SaveSession(),"Native Save crossed pending importer.");
                Require(!await session.Close()&&!session.IsClosing,"Native Close crossed pending importer.");
                Require(File.ReadAllText(disk)==diskBefore&&!vm.IsDirty,"Busy Save changed disk or dirty state.");
                var nestedLog=new LoggerResult();await vm.Sources.UpdateAssetFromSource(nestedLog);Require(nestedLog.HasErrors,"Reentrant native source update was accepted.");
                try{await bridge.Dispatch(Request("asset_set",new{assetId=vm.Id.ToString(),property="ScaleImport",value=2,expectedRevision=0}),CancellationToken.None);throw new Exception("MCP mutation crossed native importer.");}
                catch(InvalidOperationException e) when(e.Message.Contains("Native asset source update")){result["concurrentMcpRejected"]=e.Message;}
                importer.Release.Set();await updating;undo.SetName(transaction,"Native gated model reimport");
            }
            Require(!importLog.HasErrors&&!session.IsAssetOperationInProgress,"Successful source update failed or leaked busy state.");
            Require(definition.Materials.Single().Name=="ImportedMaterial"&&vm.IsDirty,"Native ModelViewModel did not update its real graph/dirty state.");
            Require(File.ReadAllText(disk)==diskBefore,"Unsaved reimport wrote to disk.");
            undo.Undo();Require(definition.Materials.Single().Name=="InitialMaterial"&&!vm.IsDirty,"Native Undo did not restore disk baseline.");
            undo.Redo();Require(vm.IsDirty&&definition.Materials.Single().Name=="ImportedMaterial","Native Redo failed.");
            Require(await session.SaveSession()&&!vm.IsDirty&&File.ReadAllText(disk).Contains("ImportedMaterial"),"Native reimport Save did not persist graph.");
            result["nativeSaveCloseDiskDirtyUndoRedo"]=true;

            foreach(string mode in new[]{"throw","partial-error"})
            {
                var hashes=HashText(definition);importer.Reset(mode);importer.Release.Set();var errors=new LoggerResult();
                using(var transaction=undo.CreateTransaction())await vm.Sources.UpdateAssetFromSource(errors);
                Require(errors.HasErrors&&!session.IsAssetOperationInProgress,"Failed importer did not release its reservation/report error.");
                Require(HashText(definition)==hashes&&definition.Materials.Single().Name=="ImportedMaterial"&&!vm.IsDirty,"Failed/partial-error importer changed graph or accepted source hashes.");
                result[mode+"GraphAndHashesUnchanged"]=true;
            }
            var terminalHashes=HashText(definition);importer.Reset("terminal");var terminalLog=new LoggerResult();
            using(var transaction=undo.CreateTransaction())
            {
                var updating=vm.Sources.UpdateAssetFromSource(terminalLog);await importer.Entered.Task.WaitAsync(TimeSpan.FromSeconds(5));
                session.ServiceProvider.Get<IAssetEditorsManager>().CloseAllEditorWindows(false);
                session.Destroy();importer.Release.Set();await updating;
            }
            Require(terminalLog.HasErrors&&!session.IsAssetOperationInProgress&&HashText(definition)==terminalHashes&&definition.Materials.Single().Name=="ImportedMaterial","Importer continued mutating graph/hashes after Destroy.");
            result["destroyDuringActualImporterAwait"]=true;result["passed"]=true;
        }
        catch(Exception error){result["passed"]=false;result["error"]=error.ToString();}
        File.WriteAllText(StartupHook.ResultPath,JsonSerializer.Serialize(result,new JsonSerializerOptions{WriteIndented=true}));
    }
    private static string HashText(Asset asset)=>string.Join(";",SourceHashesHelper.GetAllHashes(asset).OrderBy(p=>p.Key.ToString()).Select(p=>$"{p.Key}:{p.Value}"));
    private static void Require(bool condition,string message){if(!condition)throw new Exception(message);}
}

internal sealed class GatedModelImporter:ThreeDAssetImporter
{
    internal TaskCompletionSource<bool> Entered=new(TaskCreationOptions.RunContinuationsAsynchronously);
    internal ManualResetEventSlim Release=new();private string mode="success";
    public GatedModelImporter(){Order=-1000;}
    public override Guid Id=>new("2b43333c-ec41-4f44-9ce9-7a136be5b784");
    public override bool IsSupportingFile(string path)=>Path.GetFileName(path)=="native-source-probe.obj";
    internal void Reset(string value){mode=value;Entered=new(TaskCreationOptions.RunContinuationsAsynchronously);Release.Reset();}
    public override IEnumerable<AssetItem> Import(UFile path,AssetImporterParameters parameters)
    {
        Entered.TrySetResult(true);if(!Release.Wait(TimeSpan.FromSeconds(8)))throw new TimeoutException("Gated test importer was not released.");
        if(mode=="throw")throw new InvalidDataException("Deliberate native importer failure");
        if(mode=="partial-error")parameters.Logger.Error("Deliberate partial result with importer error");
        var asset=new ModelAsset{Source=path};asset.Materials.Add(new ModelMaterial{Name=mode=="success"?"ImportedMaterial":"MUST NOT MERGE",MaterialInstance=new()});
        return [new AssetItem("PreparedImport",asset)];
    }
}

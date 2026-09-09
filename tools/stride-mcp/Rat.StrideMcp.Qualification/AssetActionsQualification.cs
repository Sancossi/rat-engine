using System.IO;
using System.Text.Json;
using System.Text.Json.Nodes;
using Stride.Assets.Entities;
using Stride.Assets.Models;
using Stride.Assets.Materials;
using Stride.Core.Assets;
using Stride.Core.Assets.Editor.Services;
using Stride.Core.Assets.Editor.ViewModel;
using Stride.Core.Diagnostics;
using Stride.Core.Mathematics;
using Stride.Core.Presentation.Services;
using Stride.Engine;
using Stride.Rendering;
using Stride.Rendering.Materials;
using Stride.Rendering.Materials.ComputeColors;

namespace Rat.StrideMcp.Qualification;
internal static class AssetActionsQualification
{
    internal static async Task Run(SessionViewModel session)
    {
        var result=new Dictionary<string,object>();
        EventHandler<System.Runtime.ExceptionServices.FirstChanceExceptionEventArgs> diagnostic=(_,e)=>{
            if(e.Exception is NullReferenceException)File.AppendAllText(StartupHook.ResultPath+".exceptions.log",e.Exception+Environment.NewLine);
        };
        AppDomain.CurrentDomain.FirstChanceException+=diagnostic;
        try
        {
            await Task.Delay(1500);
            var package=session.LocalPackages.Single(p=>p.Name=="Rat.Expedition.Authoring");
            if(session.AllAssets.Any(a=>a.IsDirty))throw new InvalidOperationException("Own action fixture session is initially dirty.");
            Dictionary<string,string> ids;
            if(StartupHook.Mode=="actions-reopen")
            {
                var previous=JsonNode.Parse(File.ReadAllText(Path.Combine(Path.GetDirectoryName(StartupHook.ResultPath)!,"result.json")))!;
                ids=previous["assets"]!.Deserialize<Dictionary<string,string>>()!;
            }
            else
            {
                string folderPath=Path.Combine(package.RootDirectory.ToString(),"Assets","McpActionsQualification");
                if(Directory.Exists(folderPath))throw new InvalidOperationException("Refusing existing action fixtures.");
                var folder=package.GetOrCreateAssetDirectory("McpActionsQualification",true);
                var created=new Dictionary<string,AssetViewModel>();
                AssetViewModel Add(string name,Asset asset)
                {
                    var vm=package.CreateAssetsAtomic(folder,[new AssetItem("McpActionsQualification/"+name,asset)],new LoggerResult()).Single();
                    created.Add(name,vm);return vm;
                }
                var material=Add("Material",new MaterialAsset{Attributes=new(){Diffuse=new MaterialDiffuseMapFeature(new ComputeColor(new Color4(.3f,.6f,.2f,1))),DiffuseModel=new MaterialDiffuseLambertModelFeature()}});
                var baseMaterial=Add("BaseMaterial",new MaterialAsset());
                Add("DerivedMaterial",new MaterialAsset{Archetype=new AssetReference(baseMaterial.Id,baseMaterial.Url)});
                var model=new ModelAsset();model.Materials.Add(new(){Name="fixture",MaterialInstance=new(){Material=ContentReferenceHelper.CreateReference<Material>(material)}});
                var modelA=Add("ModelA",model);
                var modelB=new ModelAsset();modelB.Materials.Add(new(){Name="shared",MaterialInstance=new(){Material=ContentReferenceHelper.CreateReference<Material>(material)}});Add("ModelB",modelB);
                var modelC=new ModelAsset();modelC.Materials.Add(new(){Name="local",MaterialInstance=new(){Material=ContentReferenceHelper.CreateReference<Material>(baseMaterial)}});Add("ModelC",modelC);
                Add("Unreferenced",new MaterialAsset());
                var target=new SceneAsset();var targetRoot=new Entity("Cycle target part");target.Hierarchy.RootParts.Add(targetRoot);target.Hierarchy.Parts.Add(new EntityDesign(targetRoot));Add("Target",target);
                var prefab=new PrefabAsset();
                var root=new Entity("Nonzero prefab root"){new ModelComponent{Model=ContentReferenceHelper.CreateReference<Model>(modelA)}};
                root.Transform.Position=new Vector3(1,2,3);
                var child=new Entity("Internal child");child.Transform.Parent=root.Transform;
                prefab.Hierarchy.RootParts.Add(root);prefab.Hierarchy.Parts.Add(new EntityDesign(root));prefab.Hierarchy.Parts.Add(new EntityDesign(child));
                Add("Prefab",prefab);
                var extreme=new PrefabAsset();var huge=new Entity("Finite large root");huge.Transform.Position=new Vector3(float.MaxValue,0,0);extreme.Hierarchy.RootParts.Add(huge);extreme.Hierarchy.Parts.Add(new EntityDesign(huge));Add("Overflow",extreme);
                // Derive through the native API so component IDs and inheritance metadata
                // agree. A hand-built BasePart loses its unmatched Transform on reconcile.
                var derivedTarget=(SceneAsset)target.CreateDerivedAsset(created["Target"].Url);
                var cycle=new PrefabAsset{Hierarchy=derivedTarget.Hierarchy};
                Add("Cycle",cycle);
                if(cycle.Hierarchy.Parts.Values.Any(p=>p.Entity.Transform is null))throw new InvalidOperationException("Cycle fixture lost its native inherited Transform.");
                ids=created.ToDictionary(p=>p.Key,p=>p.Value.Id.ToString());
                if(!await session.SaveSession())throw new InvalidOperationException("Cannot save native action fixtures.");
            }
            result["assets"]=ids;
            int saveDenied=0,closeDenied=0;
            System.ComponentModel.PropertyChangedEventHandler busy=(_,e)=>{
                if(e.PropertyName==nameof(SessionViewModel.IsAssetOperationInProgress)&&session.IsAssetOperationInProgress){
                    var save=session.SaveSession();var close=session.Close();
                    if(!save.IsCompletedSuccessfully||save.Result||!close.IsCompletedSuccessfully||close.Result)throw new InvalidOperationException("Native Save/Close was not excluded by action lease.");
                    saveDenied++;closeDenied++;
                }
            };
            session.PropertyChanged+=busy;
            File.WriteAllText(StartupHook.ResultPath+".ready.json",JsonSerializer.Serialize(new{assets=ids,processId=Environment.ProcessId,uneditableId=session.AllAssets.First(a=>a.AssetType.Name=="ScriptSourceFileAsset").Id.ToString()}));
            var clientPath=StartupHook.ResultPath+".client.json";
            for(int i=0;!File.Exists(clientPath)&&i<480;i++)await Task.Delay(250);
            if(!File.Exists(clientPath))throw new InvalidOperationException("Action MCP client exceeded 120 seconds.");
            var client=JsonNode.Parse(File.ReadAllText(clientPath))!;
            if(!client["passed"]!.GetValue<bool>())throw new InvalidOperationException("Action MCP client failed.");
            session.PropertyChanged-=busy;
            if(session.AllAssets.Any(a=>a.IsDirty))throw new InvalidOperationException("Action client left dirty assets.");
            result["mcpClient"]=client;result["nativeSaveDenied"]=saveDenied;result["nativeCloseDenied"]=closeDenied;
            result["passed"]=true;
            session.ServiceProvider.Get<IAssetEditorsManager>().CloseAllEditorWindows(false);session.Destroy();
        }
        catch(Exception error){result["passed"]=false;result["error"]=error.ToString();}
        finally{AppDomain.CurrentDomain.FirstChanceException-=diagnostic;}
        File.WriteAllText(StartupHook.ResultPath,JsonSerializer.Serialize(result,new JsonSerializerOptions{WriteIndented=true}));
    }
}

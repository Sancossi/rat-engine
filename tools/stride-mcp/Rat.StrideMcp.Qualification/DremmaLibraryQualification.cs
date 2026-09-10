using System.IO;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.Json.Nodes;
using Stride.Assets.Entities;
using Stride.Assets.Materials;
using Stride.Assets.Models;
using Stride.Assets.Textures;
using Stride.Core.Assets;
using Stride.Core.Assets.Editor.Services;
using Stride.Core.Assets.Editor.ViewModel;
using Stride.Core.Assets.Tracking;
using Stride.Core.Diagnostics;
using Stride.Core.IO;
using Stride.Core.Mathematics;
using Stride.Core.Presentation.Services;
using Stride.Core.Serialization;
using Stride.Core.Serialization.Contents;
using Stride.Core.Storage;
using Stride.Engine;
using Stride.Graphics;
using Stride.Rendering;
using Stride.Rendering.Materials;
using Stride.Rendering.Materials.ComputeColors;

namespace Rat.StrideMcp.Qualification;

// Opt-in only. This qualifies imported copies of the real CanalCity sources in
// the normal Authoring package. It does not add import/reimport to MCP.
internal static class DremmaLibraryQualification
{
    private const string AssetFolder="DremmaLibraryQualification";
    private static readonly string[] InitialSlots=["CC_iron","CC_brass","CC_amber"];
    private static readonly string[] ReplacementSlots=["CC_stone","CC_stone_dark","CC_stone_light","CC_burgundy","CC_brass","CC_iron","CC_amber"];

    internal static async Task Run(SessionViewModel session)
    {
        var result=new Dictionary<string,object>();
        try
        {
            await Task.Delay(1500);
            var package=session.LocalPackages.Single(p=>p.Name=="Rat.Expedition.Authoring");
            var undo=session.ServiceProvider.Get<IUndoRedoService>();
            string packageRoot=package.RootDirectory.ToString();
            string resourceFolder=Path.Combine(packageRoot,"Resources",AssetFolder);
            string assetFolder=Path.Combine(packageRoot,"Assets",AssetFolder);
            string runFolder=Path.GetDirectoryName(StartupHook.ResultPath)!;
            var library=ValidateLibrary(session,package,packageRoot);
            result["library"]=library;

            if(StartupHook.Mode=="dremma-library-reopen")
            {
                Require(Directory.Exists(resourceFolder)&&Directory.Exists(assetFolder),"Saved Dremma fixture is unavailable on fresh reopen.");
                var previous=JsonNode.Parse(File.ReadAllText(Path.Combine(runFolder,"result.json")))!;
                var reopenIds=previous["assets"]!.Deserialize<Dictionary<string,string>>()!;
                var assets=Resolve(session,reopenIds);
                var expected=previous["snapshot"]!.ToJsonString();
                Require(JsonSerializer.Serialize(Snapshot(assets,resourceFolder))==expected,"Fresh editor state differs from the saved Dremma fixture.");
                Require(!session.AllAssets.Any(a=>a.IsDirty),"Freshly reopened Dremma fixture is dirty.");
                File.WriteAllText(StartupHook.ResultPath+".ready.json",JsonSerializer.Serialize(new{assets=reopenIds,processId=Environment.ProcessId}));
                await WaitFor(StartupHook.ResultPath+".client.json",60,"Fresh-reopen MCP client");
                var reopenClient=JsonNode.Parse(File.ReadAllText(StartupHook.ResultPath+".client.json"))!;
                Require(reopenClient["passed"]!.GetValue<bool>(),"Fresh-reopen MCP inspection failed.");
                result["mcpClient"]=reopenClient;result["snapshot"]=Snapshot(assets,resourceFolder);result["assets"]=reopenIds;result["passed"]=true;
                session.ServiceProvider.Get<IAssetEditorsManager>().CloseAllEditorWindows(false);session.Destroy();
                return;
            }

            Require(!Directory.Exists(resourceFolder)&&!Directory.Exists(assetFolder),"Refusing to overwrite an existing Dremma fixture.");
            Require(!session.AllAssets.Any(a=>a.IsDirty),"Dremma library session is initially dirty.");
            Directory.CreateDirectory(resourceFolder);
            string canal=Path.Combine(packageRoot,"Resources","CanalCity");
            string fbx=Path.Combine(resourceFolder,"model-source.fbx");
            string png=Path.Combine(resourceFolder,"texture-source.png");
            File.Copy(Path.Combine(canal,"models","lantern_amber.fbx"),fbx);
            File.Copy(Path.Combine(canal,"textures","amber_basecolor.png"),png);

            var created=new Dictionary<string,AssetViewModel>();
            var folder=package.GetOrCreateAssetDirectory(AssetFolder,true);
            AssetViewModel Add(string name,Asset asset)
            {
                var logger=new LoggerResult();
                var vm=package.CreateAssetsAtomic(folder,[new AssetItem(AssetFolder+"/"+name,asset)],logger).Single();
                Require(!logger.HasErrors,$"Native creation failed for {name}.");
                created.Add(name,vm);return vm;
            }
            var shared=Add("SharedMaterial",new MaterialAsset{Attributes=new(){Diffuse=new MaterialDiffuseMapFeature(new ComputeColor(new Color4(.8f,.45f,.12f,1))),DiffuseModel=new MaterialDiffuseLambertModelFeature()}});
            var textureAsset=new TextureAsset{Source=new UFile(png),IsCompressed=false};
            SourceHashesHelper.UpdateHashes(textureAsset,new Dictionary<UFile,ObjectId>{{textureAsset.Source,session.SourceTracker.GetCurrentHash(textureAsset.Source)}});
            var texture=Add("Texture",textureAsset);
            var textureReference=ContentReferenceHelper.CreateReference<Texture>(texture);
            var textured=Add("TextureMaterial",new MaterialAsset{Attributes=new(){Diffuse=new MaterialDiffuseMapFeature(new ComputeTextureColor(textureReference)),DiffuseModel=new MaterialDiffuseLambertModelFeature()}});
            var modelAsset=new ModelAsset{Source=new UFile(fbx),DeduplicateMaterials=true};
            foreach(var name in InitialSlots)modelAsset.Materials.Add(new ModelMaterial{Name=name,MaterialInstance=new(){Material=ContentReferenceHelper.CreateReference<Material>(shared)}});
            SourceHashesHelper.UpdateHashes(modelAsset,new Dictionary<UFile,ObjectId>{{modelAsset.Source,session.SourceTracker.GetCurrentHash(modelAsset.Source)}});
            var model=Add("Model",modelAsset);
            var prefabAsset=new PrefabAsset();var prefabRoot=new Entity("Shared source prefab"){new ModelComponent{Model=ContentReferenceHelper.CreateReference<Model>(model)}};
            prefabAsset.Hierarchy.RootParts.Add(prefabRoot);prefabAsset.Hierarchy.Parts.Add(new EntityDesign(prefabRoot));var prefab=Add("Prefab",prefabAsset);
            var sceneAsset=new SceneAsset();var sceneRoot=new Entity("Qualification root");sceneAsset.Hierarchy.RootParts.Add(sceneRoot);sceneAsset.Hierarchy.Parts.Add(new EntityDesign(sceneRoot));var scene=Add("Scene",sceneAsset);
            var ids=created.ToDictionary(p=>p.Key,p=>p.Value.Id.ToString());
            Require(await session.SaveSession(),"Could not save initial Dremma fixtures.");
            Require(!session.AllAssets.Any(a=>a.IsDirty),"Initial Dremma fixture save remained dirty.");
            File.WriteAllText(StartupHook.ResultPath+".ready.json",JsonSerializer.Serialize(new{assets=ids,processId=Environment.ProcessId,initialSlots=InitialSlots}));

            await WaitFor(StartupHook.ResultPath+".edit.json",90,"Dremma editor/MCP edit stage");
            var edit=JsonNode.Parse(File.ReadAllText(StartupHook.ResultPath+".edit.json"))!;
            Require(edit["passed"]!.GetValue<bool>(),"Dremma MCP edit stage failed.");
            Require(!session.AllAssets.Any(a=>a.IsDirty),"MCP edit stage was not saved.");
            Require(SceneInstances(scene,prefab.Id)==2,"MCP did not persist two shared prefab instances.");
            Require(ModelTarget((PrefabAsset)prefab.Asset)==model.Id,"Prefab model reference changed before reimport.");

            string fbxBefore=Sha256(fbx),pngBefore=Sha256(png);
            string modelHashBefore=HashText(model.Asset),textureHashBefore=HashText(texture.Asset);
            File.Copy(Path.Combine(canal,"models","bridge_arch.fbx"),fbx,true);
            File.Copy(Path.Combine(canal,"previews","foundation_modules.png"),png,true);
            Require(Sha256(fbx)!=fbxBefore&&Sha256(png)!=pngBefore,"Owned source replacements did not change both files.");
            var importLog=new LoggerResult();
            var transaction=undo.CreateTransaction();
            try
            {
                await model.Sources.UpdateAssetFromSource(importLog);
                await texture.Sources.UpdateAssetFromSource(importLog);
                Require(!importLog.HasErrors,"Real native FBX/PNG reimport reported errors: "+string.Join(" | ",importLog.Messages.Select(m=>m.ToString())));
                undo.SetName(transaction,"Qualify real CanalCity FBX and PNG reimport");transaction.Complete();
            }
            catch(Exception error)
            {
                try{undo.AbortTransaction(transaction);}catch(Exception abort){throw new AggregateException("Dremma reimport rollback failed.",error,abort);}
                throw;
            }
            Require(!session.IsAssetOperationInProgress&&!undo.TransactionInProgress,"Native reimport leaked operation state.");
            Require(model.Id.ToString()==ids["Model"]&&texture.Id.ToString()==ids["Texture"],"Native reimport changed an asset ID.");
            Require(ModelTarget((PrefabAsset)prefab.Asset)==model.Id,"Native FBX reimport broke the prefab model reference.");
            Require(((ModelAsset)model.Asset).Materials.Select(m=>m.Name).SequenceEqual(ReplacementSlots),"Real FBX reimport did not apply replacement material metadata.");
            Require(((ModelAsset)model.Asset).Materials.Single(m=>m.Name=="CC_amber").MaterialInstance?.Material is { } retained&&ReferenceId(retained)==shared.Id,"FBX material merge did not retain the shared material reference.");
            Require(textured.Dependencies.ReferencedAssets.Any(a=>a.Id==texture.Id),"PNG reimport broke its native material dependency.");
            Require(HashText(model.Asset)!=modelHashBefore&&HashText(texture.Asset)!=textureHashBefore,"Native source hashes did not accept both changed sources.");
            var reimport=new{
                fbx=new{before=fbxBefore,after=Sha256(fbx),assetId=model.Id.ToString(),sourceHashes=HashText(model.Asset),materialSlots=((ModelAsset)model.Asset).Materials.Select(m=>m.Name).ToArray()},
                png=new{before=pngBefore,after=Sha256(png),assetId=texture.Id.ToString(),sourceHashes=HashText(texture.Asset),width=1600,height=1200},
                stableReferences=true,transactionId=transaction.Id.ToString()};
            File.WriteAllText(StartupHook.ResultPath+".reimport.ready.json",JsonSerializer.Serialize(reimport));

            await WaitFor(StartupHook.ResultPath+".client.json",90,"Dremma reimport Undo/Redo MCP stage");
            var client=JsonNode.Parse(File.ReadAllText(StartupHook.ResultPath+".client.json"))!;
            Require(client["passed"]!.GetValue<bool>(),"Dremma reimport MCP stage failed.");
            Require(!session.AllAssets.Any(a=>a.IsDirty),"Final Dremma fixture was not saved.");
            Require(((ModelAsset)model.Asset).Materials.Select(m=>m.Name).SequenceEqual(ReplacementSlots),"Final Redo did not restore replacement FBX metadata.");
            Require(ModelTarget((PrefabAsset)prefab.Asset)==model.Id&&textured.Dependencies.ReferencedAssets.Any(a=>a.Id==texture.Id),"Final reimport state lost native references.");
            result["assets"]=ids;result["reimport"]=reimport;result["mcpClient"]=client;result["snapshot"]=Snapshot(created,resourceFolder);result["passed"]=true;
            session.ServiceProvider.Get<IAssetEditorsManager>().CloseAllEditorWindows(false);session.Destroy();
        }
        catch(Exception error){result["passed"]=false;result["error"]=error.ToString();}
        finally{File.WriteAllText(StartupHook.ResultPath,JsonSerializer.Serialize(result,new JsonSerializerOptions{WriteIndented=true}));}
    }

    private static object ValidateLibrary(SessionViewModel session,PackageViewModel package,string root)
    {
        var assets=session.AllAssets.Where(a=>a.Directory?.Package==package&&a.AssetItem.Location.ToString().Replace('\\','/').Contains("CanalCity/",StringComparison.Ordinal)).ToArray();
        Require(assets.Length==97,$"Current editor loaded {assets.Length}, not all 97, CanalCity assets.");
        int Count<T>() where T:Asset=>assets.Count(a=>a.Asset is T);
        Require(Count<ModelAsset>()==12&&Count<PrefabAsset>()==12&&Count<TextureAsset>()==45&&Count<MaterialAsset>()==11,"CanalCity native asset type counts differ from the approved library.");
        Require(assets.Count(a=>a.Asset is AnimationAsset)==2,"CanalCity door clip count differs from the approved library.");
        string resources=Path.Combine(root,"Resources","CanalCity");
        foreach(var path in new[]{"canal_city_foundation.blend","fonts/OFL.txt","models/lantern_amber.fbx","animations/door_open.fbx","animations/door_close.fbx"})Require(File.Exists(Path.Combine(resources,path)),"Required CanalCity source/credit is absent: "+path);
        return new{total=assets.Length,models=Count<ModelAsset>(),prefabs=Count<PrefabAsset>(),textures=Count<TextureAsset>(),materials=Count<MaterialAsset>(),animations=assets.Count(a=>a.Asset is AnimationAsset),resourceFiles=Directory.GetFiles(resources,"*",SearchOption.AllDirectories).Length};
    }
    private static Dictionary<string,AssetViewModel> Resolve(SessionViewModel session,Dictionary<string,string> ids)=>ids.ToDictionary(p=>p.Key,p=>session.AllAssets.Single(a=>a.Id.ToString()==p.Value));
    private static AssetId ModelTarget(PrefabAsset prefab)=>ReferenceId(prefab.Hierarchy.Parts.Values.Single().Entity.Get<ModelComponent>().Model!);
    private static AssetId ReferenceId(object value)=>AttachedReferenceManager.GetAttachedReference(value)?.Id??throw new InvalidOperationException("Expected native content reference is absent.");
    private static int SceneInstances(AssetViewModel scene,AssetId prefab)=>((SceneAsset)scene.Asset).Hierarchy.Parts.Values.Count(p=>p.Base?.BasePartAsset.Id==prefab);
    private static object Snapshot(IReadOnlyDictionary<string,AssetViewModel> assets,string resources)
    {
        var model=(ModelAsset)assets["Model"].Asset;var prefab=(PrefabAsset)assets["Prefab"].Asset;var scene=(SceneAsset)assets["Scene"].Asset;
        var color=(ComputeColor)((MaterialDiffuseMapFeature)((MaterialAsset)assets["SharedMaterial"].Asset).Attributes.Diffuse!).DiffuseMap!;
        return new{
            ids=assets.ToDictionary(p=>p.Key,p=>p.Value.Id.ToString()),
            model=new{source=SourceIdentity(assets["Model"],model.Source),sourceHashes=HashValues(model),materials=model.Materials.Select(m=>new{name=m.Name,target=m.MaterialInstance?.Material is { } value?AttachedReferenceManager.GetAttachedReference(value)?.Id.ToString():null}).ToArray()},
            prefabModel=ModelTarget(prefab).ToString(),
            sceneParts=scene.Hierarchy.Parts.Values.Select(p=>new{entityId=p.Entity.Id,parentId=p.Entity.Transform.Parent?.Entity.Id,baseAssetId=p.Base?.BasePartAsset.Id.ToString(),basePartId=p.Base?.BasePartId,instanceId=p.Base?.InstanceId,position=p.Entity.Transform.Position}).OrderBy(p=>p.entityId).ToArray(),
            sharedColor=color.Value,texture=new{source=SourceIdentity(assets["Texture"],((TextureAsset)assets["Texture"].Asset).Source),sourceHashes=HashValues(assets["Texture"].Asset),file=Sha256(Path.Combine(resources,"texture-source.png"))},
            sourceModel=Sha256(Path.Combine(resources,"model-source.fbx"))};
    }
    private static string HashText(Asset asset)=>string.Join(";",SourceHashesHelper.GetAllHashes(asset).OrderBy(p=>p.Key.ToString()).Select(p=>$"{p.Key}:{p.Value}"));
    private static string[] HashValues(Asset asset)=>SourceHashesHelper.GetAllHashes(asset).Values.Select(v=>v.ToString()).OrderBy(v=>v,StringComparer.Ordinal).ToArray();
    private static string SourceIdentity(AssetViewModel asset,UFile source)
    {
        string path=source.ToString();
        if(!Path.IsPathFullyQualified(path))path=Path.GetFullPath(Path.Combine(Path.GetDirectoryName(asset.AssetItem.FullPath.ToString())!,path));
        return Path.GetRelativePath(asset.Directory.Package.RootDirectory.ToString(),path).Replace('\\','/');
    }
    private static string Sha256(string path)=>Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(path))).ToLowerInvariant();
    private static async Task WaitFor(string path,int seconds,string label)
    {
        for(int i=0;i<seconds*4&&!File.Exists(path);i++)await Task.Delay(250);
        Require(File.Exists(path),label+" exceeded "+seconds+" seconds.");
    }
    private static void Require(bool condition,string message){if(!condition)throw new InvalidOperationException(message);}
}

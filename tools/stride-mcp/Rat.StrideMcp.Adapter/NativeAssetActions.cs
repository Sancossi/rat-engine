using System.IO;
using System.Text.Json.Nodes;
using Stride.Assets.Entities;
using Stride.Assets.Quantum;
using Stride.Core.Assets;
using Stride.Core.Assets.Editor.ViewModel;
using Stride.Core.Assets.Analysis;
using Stride.Assets.Presentation.ViewModel;
using Stride.Core.Mathematics;
using Stride.Core.Serialization.Contents;
using Stride.Engine;

namespace Rat.StrideMcp.Adapter;

internal sealed partial class EditorBridge
{
    private T OwnedAction<T>(string name,Func<T> action)
    {
        if(!session.TryBeginAssetOperation(out var lease))throw new InvalidOperationException("Native asset operation is busy.");
        using(lease)
        {
            var transaction=undo.CreateTransaction();
            try{var result=action();undo.SetName(transaction,name);transaction.Complete();return result;}
            catch(Exception error)
            {
                try{undo.AbortTransaction(transaction);}
                catch(Exception abort){throw new AggregateException(transaction.IsAborted?"Operation rolled back, but notification failed.":"Rollback failed; session state is uncertain.",error,abort);}
                throw;
            }
        }
    }
    private static void SafeAssetName(string name)
    {
        if(name.Length is <1 or >80||name!=name.Trim()||name.Any(c=>!(char.IsLetterOrDigit(c)||c is ' ' or '_' or '-')))
            throw new InvalidOperationException("Use one asset name of 1..80 letters, digits, spaces, underscores or hyphens.");
        if(new[]{"CON","PRN","AUX","NUL","COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9","LPT1","LPT2","LPT3","LPT4","LPT5","LPT6","LPT7","LPT8","LPT9"}.Contains(name,StringComparer.OrdinalIgnoreCase))
            throw new InvalidOperationException("Reserved Windows filename.");
    }
    private AssetViewModel[] IncomingAssets(AssetViewModel asset)=>
        (session.DependencyManager.ComputeDependencies(asset.Id,AssetDependencySearchOptions.In,ContentLinkType.Reference)?.LinksIn??[])
        .Select(link=>session.GetAssetById(link.Item.Id)).Where(a=>a is not null&&!a.IsDeleted).Distinct().ToArray()!;

    private object RenameNativeAsset(AssetViewModel asset,string name)
    {
        SafeAssetName(name);
        // Match native validation as well as Windows path equality, so native
        // Rename cannot enter its blocking invalid-name dialog for Unicode names.
        if(asset.Directory.Assets.Any(a=>a!=asset&&(string.Equals(a.Name,name,StringComparison.OrdinalIgnoreCase)||string.Equals(a.Name,name,StringComparison.InvariantCultureIgnoreCase))))throw new InvalidOperationException("Asset name already exists in this directory.");
        var disk=Path.Combine(Path.GetDirectoryName(asset.AssetItem.FullPath.ToString())!,name+Path.GetExtension(asset.AssetItem.FullPath.ToString()));
        if(name!=asset.Name&&File.Exists(disk))throw new InvalidOperationException("Target asset file already exists.");
        var incoming=IncomingAssets(asset);
        if(incoming.Any(a=>!LocalAsset(a)))throw new InvalidOperationException("Rename would update a reference outside the editable project.");
        string before=asset.Url;
        OwnedAction("MCP rename asset",()=>{
            asset.Name=name; // Nested native rename also runs reference analysis.
            if(asset.Name!=name)throw new InvalidOperationException("Native rename did not accept the requested name.");
            return true;
        });
        return new{assetId=asset.Id.ToString(),previousUrl=before,url=asset.Url,dependents=incoming.Select(a=>new{id=a.Id.ToString(),url=a.Url,dirty=a.IsDirty}).ToArray()};
    }
    private async Task<object> DeleteNativeAsset(AssetViewModel asset)
    {
        var incoming=IncomingAssets(asset);
        if(incoming.Length!=0)throw new InvalidOperationException("Referenced asset cannot be deleted. Dependents: "+string.Join(", ",incoming.Select(a=>$"{a.Id} ({a.Url})")));
        if(!asset.CanDelete(out var reason))throw new InvalidOperationException(reason);
        var id=asset.Id.ToString();var url=asset.Url;
        // Reference/CanDelete preflight above avoids native confirmation/fix dialogs.
        if(!session.TryBeginAssetOperation(out var lease))throw new InvalidOperationException("Native asset operation is busy.");
        using(lease)
        {
            var transaction=undo.CreateTransaction();
            try{
                if(!await session.DeleteItems([asset],true)||!asset.IsDeleted)throw new InvalidOperationException("Native asset deletion failed.");
                undo.SetName(transaction,"MCP delete unreferenced asset");transaction.Complete();
            }catch(Exception error){
                try{undo.AbortTransaction(transaction);}
                catch(Exception abort){throw new AggregateException(transaction.IsAborted?"Delete rolled back, but notification failed.":"Delete rollback failed; session state is uncertain.",error,abort);}
                throw;
            }
        }
        return new{assetId=id,url,deleted=true,referencePolicy="reject-referenced",undoScope="session"};
    }
    private object PlacePrefab(AssetViewModel sceneView,AssetViewModel prefabView,JsonObject args)
    {
        if(sceneView.Asset is not SceneAsset scene||prefabView.Asset is not PrefabAsset prefab||sceneView.Directory.Package!=prefabView.Directory.Package)
            throw new InvalidOperationException("Prefab placement requires a Scene and Prefab in the same editable package.");
        if(prefab.Hierarchy.Parts.Count is <1 or >256||prefab.Hierarchy.RootParts.Count is <1 or >16)throw new InvalidOperationException("Prefab hierarchy is empty or exceeds this tool's bounded size.");
        if(prefabView is not PrefabViewModel nativePrefab||nativePrefab.GatherAllBasePartAssets().Contains(sceneView))throw new InvalidOperationException("Prefab depends on the target scene; placement would create a cycle.");
        foreach(var link in AssetReferenceAnalysis.Visit(prefab))
            if(link.Reference is IReference reference&&(session.GetAssetById(reference.Id) is not {IsDeleted:false} target||!LocalAsset(target)))
                throw new InvalidOperationException("Prefab has an absent or nonlocal asset reference.");
        var seen=new HashSet<Guid>();var pending=new Queue<Entity>(prefab.Hierarchy.RootParts);
        if(prefab.Hierarchy.RootParts.Any(root=>root.Transform.Parent!=null))throw new InvalidOperationException("Prefab root has an external parent.");
        while(pending.TryDequeue(out var entity))
        {
            if(!seen.Add(entity.Id)||!prefab.Hierarchy.Parts.TryGetValue(entity.Id,out var design)||design.Entity!=entity)throw new InvalidOperationException("Prefab hierarchy contains duplicate, cyclic or external parts.");
            foreach(var child in entity.Transform.Children){if(child.Parent!=entity.Transform)throw new InvalidOperationException("Prefab child parent link is inconsistent.");pending.Enqueue(child.Entity);}
        }
        if(seen.Count!=prefab.Hierarchy.Parts.Count)throw new InvalidOperationException("Prefab contains unreachable parts.");
        foreach(var part in prefab.Hierarchy.Parts.Values)
            if(part.Base is { } inherited&&(session.GetAssetById(inherited.BasePartAsset.Id) is not {IsDeleted:false} baseAsset||!LocalAsset(baseAsset)))
                throw new InvalidOperationException("Prefab contains an absent or nonlocal base asset reference.");
        var position=(Vector3)AssetValue(args["position"],typeof(Vector3));
        // Same base-asset URL convention as native AddPrefabAssetPolicy.
        var instance=prefab.CreatePrefabInstance(prefabView.Url,out var instanceId);
        var existing=scene.Hierarchy.Parts.Values.SelectMany(p=>p.Entity.Components.Select(c=>c.Id).Append(p.Entity.Id)).ToHashSet();
        var identities=instance.Parts.Values.SelectMany(p=>p.Entity.Components.Select(c=>c.Id).Append(p.Entity.Id)).ToArray();
        if(identities.Distinct().Count()!=identities.Length||identities.Any(existing.Contains))throw new InvalidOperationException("Native prefab instance identities collided.");
        var positions=instance.RootParts.ToDictionary(root=>root.Id,root=>root.Transform.Position+position);
        if(positions.Values.Any(p=>!float.IsFinite(p.X)||!float.IsFinite(p.Y)||!float.IsFinite(p.Z)))throw new InvalidOperationException("Combined prefab root position must remain finite.");
        var graph=(EntityHierarchyPropertyGraph)sceneView.PropertyGraph;
        OwnedAction($"MCP place prefab: {prefabView.Url}",()=>
        {
            foreach(var root in instance.RootParts)
            {
                graph.AddPartToAsset(instance.Parts,instance.Parts[root.Id],null,scene.Hierarchy.RootParts.Count);
                var member=session.AssetNodeContainer.GetOrCreateNode(root.Transform)["Position"];
                member.Update(positions[root.Id]); // Native override after base graph registration.
            }
            return true;
        });
        return new{sceneId=sceneView.Id.ToString(),prefabId=prefabView.Id.ToString(),instanceId,rootEntityIds=instance.RootParts.Select(e=>e.Id).ToArray(),entityIds=instance.Parts.Keys.ToArray(),dirty=sceneView.IsDirty};
    }
}

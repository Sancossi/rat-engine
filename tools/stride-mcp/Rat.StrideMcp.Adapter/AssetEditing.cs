using System.IO;
using System.Text.Json;
using System.Text.Json.Nodes;
using Stride.Assets.Entities;
using Stride.Assets.Materials;
using Stride.Assets.Media;
using Stride.Assets.Models;
using Stride.Assets.Sprite;
using Stride.Assets.SpriteFont;
using Stride.Assets.Textures;
using Stride.Assets.UI;
using Stride.Core.Assets;
using Stride.Core.Assets.Editor.Services;
using Stride.Core.Assets.Editor.ViewModel;
using Stride.Core.Mathematics;
using Stride.Core.Quantum;
using Stride.Core.Serialization;
using Stride.Engine;
using Stride.Graphics;
using Stride.Rendering;
using Stride.Rendering.Materials;
using Stride.Rendering.Materials.ComputeColors;
using Stride.UI.Controls;

namespace Rat.StrideMcp.Adapter;

internal sealed partial class EditorBridge
{
    private static bool ResourceType(Asset asset)=>asset is SceneAsset or PrefabAsset or ModelAsset or TextureAsset or MaterialAsset or SpriteSheetAsset or SoundAsset or SpriteFontAsset or UIPageAsset;
    private string ProjectRoot=>Path.GetDirectoryName(Path.GetFullPath(session.SessionFilePath.ToString()))!;
    private bool LocalPackage(PackageViewModel package)=>package.IsEditable&&AssetPaths.Inside(ProjectRoot,package.PackagePath.ToString());
    private bool LocalAsset(AssetViewModel asset)=>asset.Directory is not null&&LocalPackage(asset.Directory.Package)&&AssetPaths.Inside(ProjectRoot,asset.AssetItem.FullPath.ToString());
    // A package has no serialized GUID in current Stride. This explicit session-scoped
    // key is its normalized path relative to the selected solution, not UI selection.
    private string PackageKey(PackageViewModel package)=>Path.GetRelativePath(ProjectRoot,package.PackagePath.ToString()).Replace('\\','/');
    private AssetViewModel OwnedAsset(JsonObject args,string key="assetId")
    {
        var id=new AssetId(Guid.Parse(args[key]!.GetValue<string>()));
        var asset=session.GetAssetById(id)??throw new InvalidOperationException("Native asset ID is absent from this session.");
        if(asset.IsDeleted||!ResourceType(asset.Asset)||!LocalAsset(asset))
            throw new InvalidOperationException("Asset is not a supported editable resource in the selected project.");
        return asset;
    }
    private object AssetCatalog()=>new {
        packages=session.LocalPackages.Where(LocalPackage).Select(p=>new {packageKey=PackageKey(p),name=p.Name}),
        assets=session.AllAssets.Where(a=>!a.IsDeleted&&ResourceType(a.Asset)&&LocalAsset(a))
            .Select(a=>new {id=a.Id.ToString(),url=a.Url,type=a.AssetType.Name,packageKey=PackageKey(a.Directory.Package),dirty=a.IsDirty}).ToArray(),
        limitation="Allowlisted asset fields/references only; no arbitrary object graph or source editing."};
    private Dictionary<string,IMemberNode> AssetFields(AssetViewModel asset,JsonObject? args=null)
    {
        var result=new Dictionary<string,IMemberNode>(StringComparer.Ordinal);
        void Add(object owner,params string[] names)
        {
            var node=session.AssetNodeContainer.GetOrCreateNode(owner);
            foreach(var name in names)if(node.TryGetChild(name) is { } member)result.Add(name,member);
        }
        switch(asset.Asset)
        {
            case TextureAsset a:Add(a,"Width","Height","IsSizeInPercentage","IsCompressed","GenerateMipmaps","IsStreamable");break;
            case ModelAsset a:Add(a,"ScaleImport","PivotPosition","MergeMeshes","DeduplicateMaterials");break;
            case SoundAsset a:Add(a,"SampleRate","CompressionRatio","StreamFromDisk","Spatialized");break;
            case SpriteSheetAsset a:
                if(args?["itemIndex"] is JsonValue index)
                {
                    int i=index.GetValue<int>();if(i<0||i>=a.Sprites.Count)throw new InvalidOperationException("Sprite item index is outside the inspected sheet.");
                    Add(a.Sprites[i],"Name","TextureRegion","Center","CenterFromMiddle","PixelsPerUnit");
                }
                else Add(a,"IsCompressed","GenerateMipmaps","PremultiplyAlpha","UseSRgbSampling","ColorKeyEnabled");
                break;
            case MaterialAsset a when a.Attributes.Diffuse is MaterialDiffuseMapFeature {DiffuseMap:ComputeColor color}:Add(color,"Value");break;
            case SpriteFontAsset a:Add(a.FontType,"Size");break;
            case UIPageAsset a when (args?["elementId"] is JsonValue element):
                var id=Guid.Parse(element.GetValue<string>());
                if(!a.Hierarchy.Parts.TryGetValue(id,out var design)||design.UIElement is not TextBlock text)throw new InvalidOperationException("UI element ID is not a TextBlock of this page.");
                Add(text,"Text","TextSize","TextColor");break;
        }
        return result;
    }
    private object InspectAsset(AssetViewModel asset)=>new {
        id=asset.Id.ToString(),url=asset.Url,type=asset.AssetType.Name,packageKey=PackageKey(asset.Directory.Package),dirty=asset.IsDirty,
        fields=AssetFields(asset).ToDictionary(p=>p.Key,p=>p.Value.Retrieve()),
        dependencies=asset.Dependencies.ReferencedAssets.Select(a=>new {id=a.Id.ToString(),url=a.Url}).ToArray(),
        referencers=asset.Dependencies.ReferencerAssets.Select(a=>new {id=a.Id.ToString(),url=a.Url}).ToArray(),
        references=References(asset),
        sprites=asset.Asset is SpriteSheetAsset sheet?sheet.Sprites.Select((s,i)=>new {itemIndex=i,s.Name,s.TextureRegion,s.Center,s.CenterFromMiddle,s.PixelsPerUnit}).ToArray():null,
        ui=asset.Asset is UIPageAsset page?page.Hierarchy.Parts.Values.Select(p=>new {id=p.UIElement.Id,type=p.UIElement.GetType().Name,text=(p.UIElement as TextBlock)?.Text}).ToArray():null,
        entities=asset.Asset is EntityHierarchyAssetBase hierarchy?hierarchy.Hierarchy.Parts.Values.Select(p=>Describe(p.Entity)).ToArray():null,
        parts=asset.Asset is EntityHierarchyAssetBase parts?parts.Hierarchy.Parts.Values.Select(p=>new{entityId=p.Entity.Id,parentId=p.Entity.Transform.Parent?.Entity.Id,baseAssetId=p.Base?.BasePartAsset.Id.ToString(),baseUrl=p.Base?.BasePartAsset.Location,basePartId=p.Base?.BasePartId,instanceId=p.Base?.InstanceId}).ToArray():null};
    private static object[] References(AssetViewModel asset)
    {
        var result=new List<object>();
        string? Id(object? value)=>value is null?null:AttachedReferenceManager.GetAttachedReference(value)?.Id.ToString();
        string? Url(object? value)=>value is null?null:AttachedReferenceManager.GetAttachedReference(value)?.Url;
        if(asset.Asset is ModelAsset model)result.AddRange(model.Materials.Select((m,i)=>(object)new{property="Material",itemIndex=i,targetAssetId=Id(m.MaterialInstance?.Material),targetUrl=Url(m.MaterialInstance?.Material)}));
        if(asset.Asset is EntityHierarchyAssetBase hierarchy)
            foreach(var e in hierarchy.Hierarchy.Parts.Values.Select(p=>p.Entity))foreach(var c in e.Components)
                if(c is ModelComponent mc)result.Add(new{entityId=e.Id,componentId=c.Id,property="Model",targetAssetId=Id(mc.Model),targetUrl=Url(mc.Model)});
                else if(c is UIComponent ui)result.Add(new{entityId=e.Id,componentId=c.Id,property="Page",targetAssetId=Id(ui.Page)});
        if(asset.Asset is UIPageAsset page)result.AddRange(page.Hierarchy.Parts.Values.Where(p=>p.UIElement is TextBlock).Select(p=>(object)new{elementId=p.UIElement.Id,property="Font",targetAssetId=Id(((TextBlock)p.UIElement).Font)}));
        return result.ToArray();
    }
    private object SetAssetField(AssetViewModel asset,JsonObject args)
    {
        var fields=AssetFields(asset,args);string key=args["property"]!.GetValue<string>();
        if(!fields.TryGetValue(key,out var member))throw new InvalidOperationException("Property is not allowlisted for this asset/item.");
        object value=AssetValue(args["value"],member.Type);
        if(asset.Asset is SoundAsset&&(key=="CompressionRatio"&&((int)value<1||(int)value>40)||key=="SampleRate"&&(int)value<=0))
            throw new InvalidOperationException("Sound CompressionRatio must be 1..40 and SampleRate must be positive.");
        if(key is "Width" or "Height" or "ScaleImport" or "PixelsPerUnit" or "Size" or "TextSize"&&Convert.ToDouble(value)<=0)
            throw new InvalidOperationException("Resource dimension/scale must be positive.");
        using(var transaction=undo.CreateTransaction())
        {member.Update(value);undo.SetName(transaction,$"MCP asset: {asset.Url}.{key}");}
        return new {assetId=asset.Id.ToString(),property=key,value=member.Retrieve(),dirty=asset.IsDirty};
    }
    private static object AssetValue(JsonNode? input,Type type)
    {
        string json=input?.ToJsonString()??"null";if(json.Length>4096)throw new InvalidOperationException("Asset value exceeds 4096 characters.");
        string[]? coordinates=type==typeof(Vector2)?["X","Y"]:type==typeof(Vector3)?["X","Y","Z"]:
            type==typeof(Color4)?["R","G","B","A"]:type==typeof(Rectangle)?["X","Y","Width","Height"]:null;
        if(coordinates is not null&&(input is not JsonObject obj||obj.Count!=coordinates.Length||coordinates.Any(c=>!obj.ContainsKey(c))))
            throw new InvalidOperationException("Every exact coordinate is required; extra fields are rejected.");
        if(!(type==typeof(string)||type==typeof(bool)||type==typeof(int)||type==typeof(float)||coordinates is not null))
            throw new InvalidOperationException("Unsupported native asset field type.");
        if(input is JsonObject members)foreach(var member in members)if(!double.IsFinite(member.Value!.GetValue<double>()))throw new InvalidOperationException("Asset coordinates must be finite.");
        var value=JsonSerializer.Deserialize(json,type,Json)??throw new InvalidOperationException("Null asset values are unsupported.");
        if(value is float f&&!float.IsFinite(f))throw new InvalidOperationException("Asset value must be finite.");
        if(value is Vector2 v2&&(!float.IsFinite(v2.X)||!float.IsFinite(v2.Y))||
           value is Vector3 v3&&(!float.IsFinite(v3.X)||!float.IsFinite(v3.Y)||!float.IsFinite(v3.Z))||
           value is Color4 color&&(!float.IsFinite(color.R)||!float.IsFinite(color.G)||!float.IsFinite(color.B)||!float.IsFinite(color.A)))
            throw new InvalidOperationException("Asset coordinates overflowed their native float representation.");
        if(value is Rectangle r&&(r.X<0||r.Y<0||r.Width<=0||r.Height<=0))throw new InvalidOperationException("Sprite region must be positive and nonnegative in source coordinates.");
        return value;
    }
    private object SetAssetReference(AssetViewModel asset,JsonObject args)
    {
        var target=OwnedAsset(args,"targetAssetId");
        if(asset.Directory.Package!=target.Directory.Package)throw new InvalidOperationException("Cross-package references are outside this resource slice.");
        string key=args["property"]!.GetValue<string>();IMemberNode member;
        if(asset.Asset is EntityHierarchyAssetBase hierarchy)
        {
            var id=Guid.Parse(args["entityId"]!.GetValue<string>());var componentId=Guid.Parse(args["componentId"]!.GetValue<string>());
            if(!hierarchy.Hierarchy.Parts.TryGetValue(id,out var design))throw new InvalidOperationException("Entity ID is absent from addressed hierarchy.");
            var component=design.Entity.Components.SingleOrDefault(c=>c.Id==componentId)??throw new InvalidOperationException("Component ID is absent from addressed entity.");
            if(!(component is ModelComponent&&key=="Model"||component is UIComponent&&key=="Page"))throw new InvalidOperationException("Only native Model and UI Page component references are supported here.");
            member=session.AssetNodeContainer.GetOrCreateNode(component)[key];
        }
        else if(asset.Asset is ModelAsset model&&key=="Material")
        {
            int index=args["itemIndex"]!.GetValue<int>();if(index<0||index>=model.Materials.Count)throw new InvalidOperationException("Model material index is outside the inspected list.");
            member=session.AssetNodeContainer.GetOrCreateNode(model.Materials[index].MaterialInstance)["Material"];
        }
        else if(asset.Asset is UIPageAsset page&&key=="Font")
        {
            var id=Guid.Parse(args["elementId"]!.GetValue<string>());
            if(!page.Hierarchy.Parts.TryGetValue(id,out var design)||design.UIElement is not TextBlock text)throw new InvalidOperationException("TextBlock ID is absent from addressed UI page.");
            member=session.AssetNodeContainer.GetOrCreateNode(text)["Font"];
        }
        else throw new InvalidOperationException("Reference field is not allowlisted for this asset.");
        var contentType=AssetRegistry.GetContentType(target.AssetType);
        if(contentType is null||!member.Type.IsAssignableFrom(contentType))throw new InvalidOperationException("Target native asset content type does not match reference field.");
        var reference=ContentReferenceHelper.CreateReference(target,member.Type)??throw new InvalidOperationException("Native content reference could not be created.");
        using(var transaction=undo.CreateTransaction())
        {member.Update(reference);undo.SetName(transaction,$"MCP reference: {asset.Url}.{key}");}
        return new {assetId=asset.Id.ToString(),property=key,targetAssetId=target.Id.ToString(),dirty=asset.IsDirty};
    }
}

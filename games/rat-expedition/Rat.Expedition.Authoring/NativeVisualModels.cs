using Rat.Expedition.Core;
using Stride.Core.Mathematics;
using Stride.Engine;
using Stride.Rendering;

namespace Rat.Expedition.Authoring;

public static class NativeVisualModels
{
    public static bool IsOccluded(SceneDefinition definition,IEnumerable<string> geometryIds,IReadOnlySet<string> hidden)
    {
        var ids=geometryIds.ToHashSet(StringComparer.Ordinal);
        return definition.OcclusionGroups.Any(group=>hidden.Contains(group.Id)&&group.Members.Any(ids.Contains));
    }
    public static void ApplyVisibility(ModelComponent component,bool authoredEnabled,bool occluded)
        =>component.Enabled=authoredEnabled&&!occluded;

    public static ModelComponent Create(ModelComponent source,IReadOnlyList<string>? selectors,string asset)
    {
        if(source.Model is null)throw new InvalidDataException($"Asset '{asset}': native visual requires a Model reference.");
        var original=source.Model;
        if(original.Meshes.Count==0)throw new InvalidDataException($"Asset '{asset}': native visual model has no meshes.");
        var selected=new HashSet<int>();
        if(selectors is {Count:>0})
        {
            if(original.Skeleton is null)throw new InvalidDataException($"Asset '{asset}': skeleton-node selectors require a model skeleton.");
            var names=original.Skeleton.Nodes.Select((node,index)=>(node.Name,index)).ToLookup(p=>p.Name,StringComparer.Ordinal);
            foreach(var name in selectors)
            {
                if(string.IsNullOrWhiteSpace(name))throw new InvalidDataException($"Asset '{asset}': skeleton-node selector is empty.");
                var matches=names[name].ToArray();
                if(matches.Length!=1)throw new InvalidDataException($"Asset '{asset}': skeleton-node selector '{name}' matched {matches.Length} nodes.");
                selected.Add(matches[0].index);
            }
        }
        var meshIndices=original.Meshes.Select((mesh,index)=>(mesh,index))
            .Where(pair=>selected.Count==0||selected.Contains(pair.mesh.NodeIndex)).Select(pair=>pair.index).ToArray();
        if(meshIndices.Length==0)throw new InvalidDataException($"Asset '{asset}': skeleton-node selection contains no meshes.");
        foreach(var index in meshIndices)
        {
            int material=original.Meshes[index].MaterialIndex;
            if(material<0||material>=original.Materials.Count)
                throw new InvalidDataException($"Asset '{asset}': selected mesh {index} has invalid material index {material} for {original.Materials.Count} slots.");
        }

        var model=original.Instantiate();
        // Model.Instantiate intentionally clones meshes/skeleton but omits material
        // slots. Share immutable loaded material instances with the borrowed draws.
        model.Materials.AddRange(original.Materials);
        model.Meshes=model.Meshes.Where((_,index)=>meshIndices.Contains(index)).ToList();
        model.BoundingSphere=original.BoundingSphere;
        var component=new ModelComponent(model){Enabled=source.Enabled,RenderGroup=source.RenderGroup,IsShadowCaster=source.IsShadowCaster};
        foreach(var material in source.Materials)component.Materials.Add(material.Key,material.Value);
        return component;
    }
}

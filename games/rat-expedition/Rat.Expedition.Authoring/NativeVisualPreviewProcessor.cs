using Stride.Core.Diagnostics;
using Stride.Engine;
using Stride.Rendering;
using System.Runtime.CompilerServices;

namespace Rat.Expedition.Authoring;

// The serialized ModelComponent remains directly editable. In the editor only,
// replace its full model with the selected borrowed-mesh view before render collect.
public sealed class NativeVisualPreviewProcessor : EntityProcessor<NativeVisualComponent,NativeVisualPreviewProcessor.Preview>
{
    public NativeVisualPreviewProcessor(){Order=-300;}
    public sealed class Preview
    {
        public ModelComponent? Component;
        public Model? Original;
        public Model? Generated;
        public string? Selection;
        public string? SourceSignature;
        public string? Error;
    }
    protected override Preview GenerateComponentData(Entity entity,NativeVisualComponent component)=>new();
    protected override bool IsAssociatedDataValid(Entity entity,NativeVisualComponent component,Preview data)=>true;
    protected override void OnEntityComponentRemoved(Entity entity,NativeVisualComponent component,Preview data)
    {
        if(data.Component is not null&&data.Original is not null&&data.Component.Model==data.Generated)data.Component.Model=data.Original;
    }
    public override void Draw(RenderContext context)
    {
        foreach(var pair in ComponentDatas)
        {
            var binding=pair.Key;var data=pair.Value;
            string? previous=data.Error;string? error=Refresh(binding,data);
            if(error is not null&&previous!=error)GlobalLogger.GetLogger("Expedition authoring").Warning(error);
        }
    }
    // Public for the small authoring regression harness; Draw uses this exact path.
    public static string? Refresh(NativeVisualComponent binding,Preview data)
    {
        var component=binding.Entity.Get<ModelComponent>();
        try
        {
            if(component is null)throw NativeSceneAdapter.Error("editor preview",binding.Entity,"Model","native visual requires ModelComponent");
            if(data.Component!=component){data.Component=component;data.Original=component.Model;Invalidate(data);}
            else if(component.Model!=data.Generated&&component.Model!=data.Original){data.Original=component.Model;Invalidate(data);}
            NativeVisualResolver.LinkedGeometryIds(binding);
            var selection=string.Join("\0",binding.SkeletonNodes??[]);
            string signature=Signature(data.Original);
            if(data.Selection!=selection||data.SourceSignature!=signature)
            {
                component.Model=data.Original;
                data.Generated=NativeVisualModels.Create(component,binding.SkeletonNodes,"editor preview").Model;
                component.Model=data.Generated;
                data.Selection=selection;
                data.SourceSignature=signature;
            }
            return data.Error=null;
        }
        catch(Exception error)
        {
            if(component is not null&&data.Original is not null)component.Model=data.Original;
            // A failed edit must not retain the successful selector/signature cache.
            // Undoing to valid authoring data will therefore rebuild the subset.
            Invalidate(data);data.Error=error.Message;return error.Message;
        }
    }
    private static void Invalidate(Preview data)
    {data.Generated=null;data.Selection=null;data.SourceSignature=null;}
    private static string Signature(Model? model)
    {
        if(model is null)return "null";
        return string.Join('|',RuntimeHelpers.GetHashCode(model),model.Materials.Count,
            string.Join(',',model.Materials.Select(RuntimeHelpers.GetHashCode)),
            string.Join(',',model.Skeleton?.Nodes.Select(n=>n.Name)??[]),
            string.Join(',',model.Meshes.Select(m=>$"{m.Name}:{m.NodeIndex}:{m.MaterialIndex}:{RuntimeHelpers.GetHashCode(m.Draw)}")));
    }
}

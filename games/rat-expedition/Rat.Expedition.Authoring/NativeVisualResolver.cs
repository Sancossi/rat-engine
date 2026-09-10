using Stride.Core;
using Stride.Core.Mathematics;
using Stride.Core.Serialization.Contents;
using Stride.Engine;

namespace Rat.Expedition.Authoring;

public sealed record NativeVisualInstance(string Id,ModelComponent Model,Vector3 Position,Quaternion Rotation,Vector3 Scale,string[] GeometryIds,bool ReplacesGeometry);

public sealed class NativeVisualLease : IDisposable
{
    private static int activeCount;
    private ContentManager? content;
    private readonly List<Scene> loaded;
    public IReadOnlyList<NativeVisualInstance> Visuals {get;}
    public static int ActiveCount=>Volatile.Read(ref activeCount);
    internal NativeVisualLease(ContentManager content,List<Scene> loaded,IReadOnlyList<NativeVisualInstance> visuals)
    {
        (this.content,this.loaded,Visuals)=(content,loaded,visuals);
        Interlocked.Increment(ref activeCount);
    }
    public void Dispose()
    {
        if(content is null)return;
        foreach(var visual in Visuals)visual.Model.Model=null;
        Exception? failure=null;
        foreach(var scene in loaded.AsEnumerable().Reverse())try{content.Unload(scene);}catch(Exception error){failure??=error;}
        loaded.Clear();content=null;Interlocked.Decrement(ref activeCount);
        if(failure is not null)throw new InvalidOperationException("Native visual content retirement failed after all resources were visited.",failure);
    }
}

public static class NativeVisualResolver
{
    public static NativeVisualLease Prepare(IServiceRegistry services,string projectAsset,string sceneId)
    {
        var content=new ContentManager(services);var loaded=new List<Scene>();
        try
        {
            var manifest=content.Load<Scene>(projectAsset);loaded.Add(manifest);
            var projects=NativeSceneAdapter.Entities(manifest,projectAsset).Where(e=>e.Get<TraversalProjectComponent>() is not null).ToArray();
            if(projects.Length!=1)throw new InvalidDataException($"Asset '{projectAsset}': requires exactly one Expedition project component.");
            var references=projects[0].Get<TraversalProjectComponent>().Scenes??[];
            Scene? selected=null;string? selectedUrl=null;
            foreach(var reference in references)
            {
                if(reference is null||string.IsNullOrWhiteSpace(reference.Url))throw new InvalidDataException($"Asset '{projectAsset}': empty scene reference.");
                var scene=content.Load<Scene>(NativeProjectLoader.ResolveContentUrl(reference.Url));loaded.Add(scene);
                var roots=NativeSceneAdapter.Entities(scene,reference.Url).Where(e=>e.Get<TraversalSceneComponent>() is not null).ToArray();
                if(roots.Length!=1)throw new InvalidDataException($"Asset '{reference.Url}': requires exactly one Expedition scene component.");
                if(roots[0].Id.ToString()==sceneId){selected=scene;selectedUrl=reference.Url;}
            }
            if(selected is null)throw new InvalidDataException($"Asset '{projectAsset}': no native scene matches candidate '{sceneId}'.");
            var entities=NativeSceneAdapter.Entities(selected,selectedUrl!);var owned=entities.ToDictionary(e=>e.Id);
            void UpdateWorld(Entity entity){entity.Transform.UpdateWorldMatrix();foreach(var child in entity.Transform.Children)UpdateWorld(child.Entity);}
            foreach(var root in selected.Entities)UpdateWorld(root);
            var visuals=new List<NativeVisualInstance>();
            foreach(var entity in entities.Where(e=>e.Get<NativeVisualComponent>() is not null))
            {
                var binding=entity.Get<NativeVisualComponent>();var model=entity.Get<ModelComponent>();
                if(model is null)throw NativeSceneAdapter.Error(selectedUrl!,entity,"Model","native visual requires ModelComponent");
                if(!TryGetSupportedWorldTransform(entity.Transform.WorldMatrix,out var scale,out var rotation,out var position))
                    throw NativeSceneAdapter.Error(selectedUrl!,entity,"Transform","native visual world transform must be finite, nondegenerate TRS without shear");
                if(binding.SkeletonNodes is null)throw NativeSceneAdapter.Error(selectedUrl!,entity,"SkeletonNodes","collection is required");
                var geometryIds=LinkedGeometryIds(binding);
                foreach(var geometryId in geometryIds)
                    if(!owned.TryGetValue(geometryId,out var geometry)||geometry.Get<GeometryComponent>() is null)
                        throw NativeSceneAdapter.Error(selectedUrl!,entity,"GeometryIds",$"'{geometryId}' is missing or not Expedition geometry");
                if(binding.ReplacesGeometry&&geometryIds.Length==0)throw NativeSceneAdapter.Error(selectedUrl!,entity,"ReplacesGeometry","requires a linked geometry id");
                visuals.Add(new(entity.Id.ToString(),NativeVisualModels.Create(model,binding.SkeletonNodes,selectedUrl!),position,
                    rotation,scale,geometryIds.Select(id=>id.ToString()).ToArray(),binding.ReplacesGeometry));
            }
            return new(content,loaded,visuals);
        }
        catch
        {
            foreach(var scene in loaded.AsEnumerable().Reverse())content.Unload(scene);
            throw;
        }
    }
    public static Guid[] LinkedGeometryIds(NativeVisualComponent binding)
    {
        if(binding.AdditionalGeometryIds is null)throw NativeSceneAdapter.Error("native visual",binding.Entity,"AdditionalGeometryIds","collection is required");
        var result=(binding.GeometryId==Guid.Empty?Enumerable.Empty<Guid>():[binding.GeometryId]).Concat(binding.AdditionalGeometryIds).ToArray();
        if(result.Any(id=>id==Guid.Empty))throw NativeSceneAdapter.Error("native visual",binding.Entity,"AdditionalGeometryIds","cannot contain an empty identity");
        if(result.Distinct().Count()!=result.Length)throw NativeSceneAdapter.Error("native visual",binding.Entity,"GeometryIds","contains a duplicate identity");
        return result;
    }
    public static bool TryGetSupportedWorldTransform(Matrix world,out Vector3 scale,out Quaternion rotation,out Vector3 position)
    {
        if(!world.Decompose(out scale,out rotation,out position)||!NativeSceneAdapter.Finite(position)||!NativeSceneAdapter.Finite(scale)||
            !float.IsFinite(rotation.X)||!float.IsFinite(rotation.Y)||!float.IsFinite(rotation.Z)||!float.IsFinite(rotation.W)||
            MathF.Abs(scale.X)<1e-5f||MathF.Abs(scale.Y)<1e-5f||MathF.Abs(scale.Z)<1e-5f)
            return false;
        rotation.Normalize();
        var reconstructed=Matrix.Scaling(scale)*Matrix.RotationQuaternion(rotation)*Matrix.Translation(position);
        ReadOnlySpan<float> actual=[world.M11,world.M12,world.M13,world.M14,world.M21,world.M22,world.M23,world.M24,
            world.M31,world.M32,world.M33,world.M34,world.M41,world.M42,world.M43,world.M44];
        ReadOnlySpan<float> expected=[reconstructed.M11,reconstructed.M12,reconstructed.M13,reconstructed.M14,
            reconstructed.M21,reconstructed.M22,reconstructed.M23,reconstructed.M24,reconstructed.M31,reconstructed.M32,
            reconstructed.M33,reconstructed.M34,reconstructed.M41,reconstructed.M42,reconstructed.M43,reconstructed.M44];
        for(int i=0;i<actual.Length;i++)
            if(!float.IsFinite(actual[i])||MathF.Abs(actual[i]-expected[i])>1e-4f*MathF.Max(1,MathF.Abs(actual[i])))return false;
        return true;
    }
}

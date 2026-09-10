using Stride.Core;
using Stride.Core.Diagnostics;
using Stride.Core.Mathematics;
using Stride.Engine;
using Stride.Graphics;
using Stride.Rendering;
using Stride.Rendering.Materials;
using Stride.Rendering.Materials.ComputeColors;
using Stride.Rendering.ProceduralModels;

namespace Rat.Expedition.Authoring;

// Runs only in the editor's scene instance. The authoring asset stores no generated
// ModelComponent/buffers; the game constructs its owned presentation from Core data.
public sealed class GeometryPreviewProcessor : EntityProcessor<GeometryComponent,GeometryPreviewProcessor.Preview>
{
    // EntityManager draws processors in order. Generate/replace models before
    // TransformProcessor(-200) updates skeleton/world matrices and before
    // ModelRenderProcessor(0) collects this frame's RenderMeshes. Never retire
    // a buffer after that collection has retained it for the same frame.
    public GeometryPreviewProcessor(){Order=-300;}
    public sealed class Preview
    {
        public ModelComponent Model {get;}=new();
        public List<Stride.Graphics.Buffer> Buffers {get;}=[];
        public (GeometryRole,Vector3,Rat.Expedition.Core.RampAxis,float)? Shape;
        public string? Error;
        public void Clear(){Model.Model=null;foreach(var buffer in Buffers.Distinct())buffer.Dispose();Buffers.Clear();Shape=null;}
    }
    protected override Preview GenerateComponentData(Entity entity,GeometryComponent component)=>new();
    protected override bool IsAssociatedDataValid(Entity entity,GeometryComponent component,Preview data)=>true;
    protected override void OnEntityComponentAdding(Entity entity,GeometryComponent component,Preview data)=>entity.Add(data.Model);
    protected override void OnEntityComponentRemoved(Entity entity,GeometryComponent component,Preview data){entity.Remove(data.Model);data.Clear();}
    public override void Draw(RenderContext context)
    {
        foreach(var pair in ComponentDatas)
        {
            var component=pair.Key;var data=pair.Value;
            try
            {
                NativeSceneAdapter.Geometry(component,"editor preview");
                var shape=(component.Role,component.Size,component.Axis,component.Rise);
                if(data.Shape!=shape)
                {
                    data.Clear();
                    PrimitiveProceduralModelBase primitive=component.Role==GeometryRole.Ramp?
                        new FiniteRampPrimitive(new(component.Entity.Id.ToString(),System.Numerics.Vector2.Zero,new(component.Size.X,component.Size.Z),component.Axis,0,component.Rise,component.Size.Y)):
                        new CubeProceduralModel{Size=component.Size};
                    var color=component.Role==GeometryRole.Floor?new Color4(.24f,.29f,.28f,1):new Color4(.40f,.43f,.44f,1);
                    var material=Material.New(context.GraphicsDevice,new MaterialDescriptor{Attributes={Diffuse=new MaterialDiffuseMapFeature(new ComputeColor(color)),DiffuseModel=new MaterialDiffuseLambertModelFeature()}});
                    primitive.SetMaterial("Material",material);var model=primitive.Generate(Services);
                    foreach(var mesh in model.Meshes)
                    {
                        foreach(var vertex in mesh.Draw.VertexBuffers)data.Buffers.Add(vertex.Buffer);
                        if(mesh.Draw.IndexBuffer is not null)data.Buffers.Add(mesh.Draw.IndexBuffer.Buffer);
                    }
                    data.Model.Model=model;data.Shape=shape;
                }
                data.Model.Enabled=!component.Entity.Scene.Entities.SelectMany(e=>Descendants(e)).Any(e=>{
                    var visual=e.Get<NativeVisualComponent>();return visual is not null&&visual.ReplacesGeometry&&NativeVisualResolver.LinkedGeometryIds(visual).Contains(component.Entity.Id);
                });data.Error=null;
            }
            catch(Exception error)
            {
                data.Model.Enabled=false;
                if(data.Error!=error.Message)GlobalLogger.GetLogger("Expedition authoring").Warning(error.Message);
                data.Error=error.Message;
            }
        }
    }
    private static IEnumerable<Entity> Descendants(Entity root)
    {
        yield return root;
        foreach(var child in root.Transform.Children)foreach(var entity in Descendants(child.Entity))yield return entity;
    }
}

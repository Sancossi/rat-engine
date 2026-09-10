using Rat.Expedition.Core;
using Rat.Expedition.Authoring;
using Stride.Core;
using Stride.Core.Mathematics;
using Stride.Engine;
using Stride.Engine.Processors;
using Stride.Graphics;
using Stride.Rendering;
using Stride.Rendering.Colors;
using Stride.Rendering.Lights;
using Stride.Rendering.Materials;
using Stride.Rendering.Materials.ComputeColors;
using Stride.Rendering.ProceduralModels;
using Stride.Rendering.Sprites;

namespace Rat.Expedition.Windows;

// A candidate owns every generated mesh buffer and its SceneInstance. Atlas/font
// belong to the game. SceneInstance alone does not dispose procedural model buffers.
internal sealed class ScenePresentation : IDisposable
{
    private readonly List<Stride.Graphics.Buffer> buffers=[];
    private readonly Dictionary<ModelComponent,bool> authoredModelEnabled=[];
    private NativeVisualLease? native;
    public SceneInstance Instance {get;private set;}=null!;
    public CameraComponent Camera {get;private set;}=null!;
    public Entity CameraEntity {get;private set;}=null!;
    public Entity[] Actors {get;}=new Entity[3];
    public SpriteFromSheet[] Providers {get;}=new SpriteFromSheet[3];
    public Dictionary<string,List<ModelComponent>> Models {get;}=new(StringComparer.Ordinal);
    public LocalOcclusion Occlusion {get;private set;}=null!;
    public int BufferCount=>buffers.Count;
    public int NativeVisualCount=>native?.Visuals.Count??0;
    public int NativeMeshCount=>native?.Visuals.Sum(v=>v.Model.Model.Meshes.Count)??0;
    public int NativeMaterialSlotCount=>native?.Visuals.Sum(v=>v.Model.Model.Materials.Count)??0;

    public static ScenePresentation Prepare(Game game,SceneChangeCandidate candidate,SpriteSheet sheet,CameraComponent template,string nativeProject)
    {
        var bundle=new ScenePresentation();
        try{bundle.Build(game,candidate,sheet,template,nativeProject);return bundle;}
        catch{bundle.Dispose();throw;}
    }
    private void Build(Game game,SceneChangeCandidate candidate,SpriteSheet sheet,CameraComponent template,string nativeProject)
    {
        var definition=candidate.Scene;var scene=new Scene();Occlusion=new(definition);
        native=NativeVisualResolver.Prepare(game.Services,nativeProject,definition.Id);
        var replaced=native.Visuals.Where(v=>v.ReplacesGeometry).SelectMany(v=>v.GeometryIds).ToHashSet(StringComparer.Ordinal);
        Camera=new(){Projection=CameraProjectionMode.Orthographic,OrthographicSize=template.OrthographicSize,NearClipPlane=.1f,FarClipPlane=100,Slot=template.Slot};
        CameraEntity=new Entity("Following orthographic camera"){Camera};
        CameraEntity.Transform.Rotation=Quaternion.RotationMatrix(Matrix.Invert(Matrix.LookAtRH(ExpeditionGame.CameraOffset,Vector3.Zero,Vector3.UnitY)));
        scene.Entities.Add(CameraEntity);
        scene.Entities.Add(new Entity("Ambient"){new LightComponent{Type=new LightAmbient{Color=new ColorRgbProvider(Color.White)},Intensity=.55f}});
        var sun=new Entity("Field light"){new LightComponent{Type=new LightDirectional{Color=new ColorRgbProvider(Color.White)},Intensity=.65f}};
        sun.Transform.RotationEulerXYZ=new(-MathUtil.PiOverFour,-MathUtil.PiOverFour,0);scene.Entities.Add(sun);
        void Add(string id,PrimitiveProceduralModelBase primitive,Vector3 position,Color4 color)
        {
            var material=Material.New(game.GraphicsDevice,new MaterialDescriptor{Attributes={Diffuse=new MaterialDiffuseMapFeature(new ComputeColor(color)),DiffuseModel=new MaterialDiffuseLambertModelFeature()}},game.Content);
            primitive.SetMaterial("Material",material);var model=primitive.Generate(game.Services);
            foreach(var mesh in model.Meshes)
            {
                foreach(var vertex in mesh.Draw.VertexBuffers)buffers.Add(vertex.Buffer);
                if(mesh.Draw.IndexBuffer is not null)buffers.Add(mesh.Draw.IndexBuffer.Buffer);
            }
            var component=new ModelComponent(model);Models.Add(id,[component]);
            authoredModelEnabled.Add(component,component.Enabled);
            var entity=new Entity(id){component};entity.Transform.Position=position;scene.Entities.Add(entity);
        }
        foreach(var box in definition.AllSolids.Concat(definition.Decorations).Concat(LadderDecorations(definition)).Where(box=>!replaced.Contains(box.Id)))
        {
            var color=box.Id.StartsWith("ladder-decoration-")?new Color4(.65f,.39f,.15f,1):box==definition.Floor?new(.24f,.29f,.28f,1):new Color4(.40f,.43f,.44f,1);
            var size=new Vector3(box.Max.X-box.Min.X,box.Max.Y-box.Min.Y,box.Max.Z-box.Min.Z);
            Add(box.Id,new CubeProceduralModel{Size=size},new Vector3(box.Min.X,box.Min.Y,box.Min.Z)+size/2,color);
        }
        foreach(var ramp in definition.Ramps.Where(ramp=>!replaced.Contains(ramp.Id)))Add(ramp.Id,new FiniteRampPrimitive(ramp.ToWorld()),Vector3.Zero,new(.34f,.38f,.39f,1));
        foreach(var visual in native.Visuals)
        {
            var entity=new Entity("native-"+visual.Id){visual.Model};
            entity.Transform.Position=visual.Position;entity.Transform.Rotation=visual.Rotation;entity.Transform.Scale=visual.Scale;
            scene.Entities.Add(entity);
            authoredModelEnabled.Add(visual.Model,visual.Model.Enabled);
            foreach(var id in visual.GeometryIds)
            {
                if(!Models.TryGetValue(id,out var models))Models.Add(id,models=[]);
                models.Add(visual.Model);
            }
        }
        foreach(var portal in definition.Portals)
            Add("portal-marker-"+portal.Id,new CubeProceduralModel{Size=new(.65f,.025f,.65f)},new(portal.Anchor.X,portal.Anchor.Y+.013f,portal.Anchor.Z),new(.25f,.48f,.62f,1));
        Color4[] tints=[new(1,1,1,1),new(.55f,.85f,1,1),new(1,.68f,.48f,1)];
        for(int i=0;i<Actors.Length;i++)
        {
            Providers[i]=new(){Sheet=sheet};Actors[i]=new Entity(i==0?"Rat leader":$"Rat companion {i}")
            {new SpriteComponent{SpriteProvider=Providers[i],SpriteType=SpriteType.Sprite,Sampler=SpriteSampler.PointClamp,IgnoreDepth=false,PremultipliedAlpha=false,IsAlphaCutoff=true,Color=tints[i]}};
            Actors[i].Transform.Rotation=Quaternion.RotationY(MathUtil.PiOverFour);
            Actors[i].Transform.Scale=new(1,ExpeditionGame.UprightScale,1);
            var p=candidate.Leader.Position;Actors[i].Transform.Position=new(p.X,p.Y,p.Z);scene.Entities.Add(Actors[i]);
        }
        Instance=new SceneInstance(game.Services,scene);
    }
    public void Dispose()
    {
        if(Instance is not null){((IReferencable)Instance).Release();Instance=null!;}
        native?.Dispose();native=null;
        foreach(var buffer in buffers.Distinct())buffer.Dispose();buffers.Clear();
    }
    public void ApplyOcclusion(SceneDefinition definition,IReadOnlySet<string> hidden)
    {
        foreach(var model in Models.Values.SelectMany(m=>m).Distinct())
        {
            var ids=Models.Where(pair=>pair.Value.Contains(model)).Select(pair=>pair.Key).ToHashSet(StringComparer.Ordinal);
            bool occluded=NativeVisualModels.IsOccluded(definition,ids,hidden);
            NativeVisualModels.ApplyVisibility(model,authoredModelEnabled[model],occluded);
        }
    }
    private static IEnumerable<WorldBox> LadderDecorations(SceneDefinition definition)
    {
        foreach(var ladder in definition.Ladders)
        {
            float z=ladder.Bottom.Z-.25f;
            foreach(float side in new[]{-.3f,.3f})yield return new($"ladder-decoration-{ladder.Id}-{side}",new(ladder.Bottom.X+side-.025f,ladder.Bottom.Y,z-.025f),new(ladder.Top.X+side+.025f,ladder.Top.Y+.25f,z+.025f));
            int count=Math.Max(0,(int)Math.Floor(((double)ladder.Top.Y-ladder.Bottom.Y-.15)/.2)+1);
            for(int i=0;i<count;i++){float y=ladder.Bottom.Y+(float)(.15+i*.2);yield return new($"ladder-decoration-{ladder.Id}-rung-{i}",new(ladder.Bottom.X-.3f,y,z-.025f),new(ladder.Top.X+.3f,y+.035f,z+.025f));}
        }
    }
}

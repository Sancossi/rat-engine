using Rat.Expedition.Authoring;
using Rat.Expedition.Core;
using Stride.Core.Mathematics;
using Stride.Engine;
using Stride.Rendering;

int passed=0,failed=0;
void Check(string name,Action test){try{test();passed++;Console.WriteLine("PASS "+name);}catch(Exception e){failed++;Console.WriteLine("FAIL "+name+": "+e);}}
void Require(bool result){if(!result)throw new Exception("Observable assertion failed.");}
void Reject(Action test,string diagnostic){try{test();}catch(InvalidDataException e){Require(e.Message.Contains(diagnostic,StringComparison.OrdinalIgnoreCase));return;}throw new Exception("Expected rejection: "+diagnostic);}
(Scene Scene,Entity Root,Entity Floor,Entity Spawn,Entity Wall) Fixture()
{
    var scene=new Scene();var spawn=new Entity("Arrival point"){new SpawnComponent()};spawn.Transform.Position=new(0,0,2);
    var root=new Entity("fixture"){new TraversalSceneComponent{DefaultSpawn=spawn}};
    var floor=new Entity("Ground"){new GeometryComponent{Role=GeometryRole.Floor,Size=new(10,.3f,10)}};floor.Transform.Position=new(0,-.15f,0);
    var wall=new Entity("Wall"){new GeometryComponent{Role=GeometryRole.Wall,Size=new(2,1,.5f)}};wall.Transform.Position=new(0,.5f,0);
    foreach(var e in new[]{root,floor,spawn,wall})scene.Entities.Add(e);
    return(scene,root,floor,spawn,wall);
}
SceneDefinition Read(Scene scene)=>NativeSceneAdapter.Convert(scene,"Fixture",new Dictionary<string,Scene>{{"Fixture",scene}});
Check("native default spawn reference and rename preserve identity",()=>{
    var f=Fixture();var before=Read(f.Scene);f.Spawn.Name="Renamed arrival";f.Wall.Name="Renamed wall";var after=Read(f.Scene);
    Require(before.DefaultSpawnId==f.Spawn.Id.ToString()&&before.Spawn==after.Spawn&&before.Walls[0]==after.Walls[0]);
});
Check("moving one authored wall changes old and new collision",()=>{
    var f=Fixture();var before=Read(f.Scene);f.Wall.Transform.Position.X=3;var after=Read(f.Scene);
    Require(!before.CreateWorld().HasClearance(new(0,0,0),.2f,.8f)&&after.CreateWorld().HasClearance(new(0,0,0),.2f,.8f));
    Require(before.CreateWorld().HasClearance(new(3,0,0),.2f,.8f)&&!after.CreateWorld().HasClearance(new(3,0,0),.2f,.8f));
});
Check("zero negative nonfinite size rejected at authored entity",()=>{
    foreach(float value in new[]{0,-1,float.NaN,float.PositiveInfinity}){var f=Fixture();f.Wall.Get<GeometryComponent>().Size=new(value,1,.5f);Reject(()=>Read(f.Scene),"Size");}
});
Check("rotation scale matrix and inherited rotation rejected",()=>{
    foreach(int variant in Enumerable.Range(0,5)){
        var f=Fixture();var parent=new Entity("Rotated parent");f.Scene.Entities.Remove(f.Wall);parent.Transform.Children.Add(f.Wall.Transform);f.Scene.Entities.Add(parent);
        if(variant==0)f.Wall.Transform.Rotation=Quaternion.RotationY(.1f);
        if(variant==1)f.Wall.Transform.Scale=new(2,1,1);
        if(variant==2)f.Wall.Transform.UseTRS=false;
        if(variant==3)parent.Transform.Rotation=Quaternion.RotationZ(.1f);
        if(variant==4)parent.Transform.Scale=new(-1,1,1);
        Reject(()=>Read(f.Scene),"Transform");
    }
});
Check("translated parent contributes once to world bounds",()=>{
    var f=Fixture();var parent=new Entity("Translated parent");parent.Transform.Position=new(2,0,0);f.Scene.Entities.Remove(f.Wall);parent.Transform.Children.Add(f.Wall.Transform);f.Scene.Entities.Add(parent);
    var scene=Read(f.Scene);Require(scene.Walls[0].Min.X==1&&scene.Walls[0].Max.X==3);
});
Check("common fractional vertical parent preserves floor wall and spawn contact",()=>{
    foreach(float y in new[]{.1f,-.1f,.3f,1000f}){
    var f=Fixture();var parent=new Entity("Common translated parent");parent.Transform.Position=new(.1f,y,.1f);
    foreach(var entity in f.Scene.Entities.ToArray()){f.Scene.Entities.Remove(entity);parent.Transform.Children.Add(entity.Transform);}
    f.Scene.Entities.Add(parent);
    var scene=Read(f.Scene);
    Require(scene.Floor.Max.Y==y&&scene.Walls[0].Min.Y==scene.Floor.Max.Y&&scene.Spawn.Y==y);
    Require(scene.CreateWorld().HasSupport(scene.Spawn.Vector,.2f)&&scene.CreateWorld().HasClearance(scene.Spawn.Vector,.2f,.8f));
    f.Wall.Transform.Position.Y+=.01f;
    Reject(()=>Read(f.Scene),"wall bottom must meet floor top");
    }
});
Check("duplicate native identity rejected",()=>{var f=Fixture();f.Wall.Id=f.Floor.Id;Reject(()=>Read(f.Scene),"identity");});
Check("missing foreign and non-spawn default references rejected",()=>{
    foreach(int variant in Enumerable.Range(0,3)){var f=Fixture();f.Root.Get<TraversalSceneComponent>().DefaultSpawn=variant==0?null:variant==1?new Entity("Foreign"){new SpawnComponent()}:f.Wall;Reject(()=>Read(f.Scene),"DefaultSpawn");}
});
Check("unsafe spawn reports asset and spawn identity",()=>{var f=Fixture();f.Spawn.Transform.Position=Vector3.Zero;Reject(()=>Read(f.Scene),f.Spawn.Id.ToString());});
Check("missing ladder point and cross-scene point rejected",()=>{
    var f=Fixture();var ladder=new Entity("Ladder"){new LadderComponent()};f.Scene.Entities.Add(ladder);Reject(()=>Read(f.Scene),"Bottom");
    ladder.Get<LadderComponent>().Bottom=new Entity("foreign");Reject(()=>Read(f.Scene),"Bottom");
});
Check("missing portal scene and spawn rejected",()=>{
    var f=Fixture();var portal=new Entity("Gate"){new PortalComponent()};portal.Transform.Position=new(3,0,2);f.Scene.Entities.Add(portal);Reject(()=>Read(f.Scene),"TargetScene");
    portal.Get<PortalComponent>().TargetScene=new("Fixture");portal.Get<PortalComponent>().TargetSpawnId=Guid.NewGuid();Reject(()=>Read(f.Scene),"TargetSpawnId");
});
Check("deleted occlusion reference and wrong parent rejected",()=>{
    var f=Fixture();var group=new Entity("Cut"){new OcclusionComponent{Members=[new Entity("Deleted")]}};f.Scene.Entities.Add(group);Reject(()=>Read(f.Scene),"Members");
    group.Get<OcclusionComponent>().Members=[f.Wall];group.Get<OcclusionComponent>().HideWith=f.Spawn;Reject(()=>Read(f.Scene),"HideWith");
});
Check("finite ramp schema preserves slab top and rejects degenerate metadata",()=>{
    var f=Fixture();var ramp=new Entity("Ramp"){new GeometryComponent{Role=GeometryRole.Ramp,Size=new(2,.2f,1),Rise=1}};ramp.Transform.Position=new(2,0,-2);f.Scene.Entities.Add(ramp);
    var r=Read(f.Scene).Ramps.Single();Require(r.StartY==0&&r.EndY==1&&r.Thickness==.2f&&r.Min.X==2&&r.Max.X==4);
    ramp.Get<GeometryComponent>().Rise=float.NaN;Reject(()=>Read(f.Scene),"Ramp");
});
Check("native prefab instances receive distinct entity ids and remap local links",()=>{
    var child=new Entity("Spawn"){new SpawnComponent()};var root=new Entity("Root"){new TraversalSceneComponent{DefaultSpawn=child}};root.Transform.Children.Add(child.Transform);
    var prefab=new Prefab();prefab.Entities.Add(root);var a=NativePrefabInstances.Instantiate(prefab).Single();var b=NativePrefabInstances.Instantiate(prefab).Single();
    Require(a.Id!=b.Id&&a.Id!=root.Id&&a.Get<TraversalSceneComponent>().DefaultSpawn==a.Transform.Children.Single().Entity&&b.Get<TraversalSceneComponent>().DefaultSpawn==b.Transform.Children.Single().Entity);
    var aIds=new[]{a.Id,a.Transform.Children.Single().Entity.Id};
    var bIds=new[]{b.Id,b.Transform.Children.Single().Entity.Id};
    Require(aIds.Distinct().Count()==2&&bIds.Distinct().Count()==2&&!aIds.Intersect(bIds).Any()&&!aIds.Contains(child.Id)&&!bIds.Contains(child.Id));
});
Check("native visual subsets retain shared materials and independent overrides",()=>{
    var first=new Material();var second=new Material();var local=new Material();
    var model=new Model{Skeleton=new Skeleton{Nodes=[new(){Name="Root",ParentIndex=-1},new(){Name="deck",ParentIndex=0},new(){Name="posts",ParentIndex=0}]}};
    model.Materials.Add(new(first));model.Materials.Add(new(second));
    model.Meshes.Add(new(){Name="deck-a",NodeIndex=1,MaterialIndex=0});
    model.Meshes.Add(new(){Name="deck-b",NodeIndex=1,MaterialIndex=1});
    model.Meshes.Add(new(){Name="posts",NodeIndex=2,MaterialIndex=1});
    var source=new ModelComponent(model);source.Materials.Add(1,local);
    var deck=NativeVisualModels.Create(source,["deck"],"fixture");
    var posts=NativeVisualModels.Create(source,["posts"],"fixture");
    Require(deck.Model.Meshes.Count==2&&posts.Model.Meshes.Count==1&&model.Meshes.Count==3);
    Require(deck.Model.Skeleton==model.Skeleton&&deck.Model.Materials.Count==2&&ReferenceEquals(deck.Model.Materials[0],model.Materials[0]));
    Require(deck.Materials[1]==local&&posts.Materials[1]==local&&!ReferenceEquals(deck.Model,posts.Model));
    Reject(()=>NativeVisualModels.Create(source,["absent"],"fixture"),"selector");
});
Check("native visual invalid material slot fails before presentation",()=>{
    var model=new Model{Skeleton=new Skeleton{Nodes=[new(){Name="Root",ParentIndex=-1}]}};
    model.Meshes.Add(new(){Name="bad",NodeIndex=0,MaterialIndex=2});
    Reject(()=>NativeVisualModels.Create(new(model),[],"fixture"),"material index");
});
Check("candidate source callback failure retains active world and position",()=>{
    var f=Fixture();var portal=new Entity("Return gate"){new PortalComponent{TargetScene=new("Fixture")}};
    portal.Transform.Position=f.Spawn.Transform.Position+new Vector3(1.2f,0,0);f.Scene.Entities.Add(portal);
    var data=Read(f.Scene);int calls=0,prepared=0,activated=0;
    var project=ExpeditionProject.FromSource(data.Id,[data],id=>{calls++;if(calls>1)throw new InvalidDataException("native source missing");return data;});
    var session=new ExpeditionSession(project,_=>{prepared++;return new Prepared(()=>activated++);});
    var input=new System.Numerics.Vector2(TraversalMotor.CameraRight.X,TraversalMotor.CameraForward.X);
    for(int i=0;i<48;i++)session.Advance(TraversalMotor.StepSeconds,new(input));
    var before=session.Snapshot;var oldScene=session.Scene;var party=session.Trail.Companions.ToArray();
    session.Advance(TraversalMotor.StepSeconds,new(System.Numerics.Vector2.Zero,InteractHeld:true));
    Require(calls==2&&prepared==1&&activated==1&&session.WorldRevision==0&&ReferenceEquals(session.Scene,oldScene));
    Require(session.Leader.Position==before.Leader.Position&&session.Leader.State==before.Leader.State&&session.SafePoint==before.SafePoint);
    Require(session.LastError!.Contains("native source missing")&&session.Trail.Companions.SequenceEqual(party));
});
Console.WriteLine($"Authoring scenarios: {passed} passed, {failed} failed.");
return failed==0?0:1;

sealed class Prepared(Action activate):IPreparedSceneChange
{
    public void Activate()=>activate();
    public void Dispose(){}
}

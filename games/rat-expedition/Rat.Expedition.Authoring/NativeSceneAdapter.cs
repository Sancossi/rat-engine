using Rat.Expedition.Core;
using Stride.Core.Mathematics;
using Stride.Engine;

namespace Rat.Expedition.Authoring;

public static class NativeSceneAdapter
{
    public static InvalidDataException Error(string asset,Entity entity,string property,string reason)
        =>new($"Asset '{asset}', entity '{entity.Name}' ({entity.Id}), {property}: {reason}");

    public static Entity[] Entities(Scene scene,string asset)
    {
        if(scene.Children.Count!=0||scene.Offset!=Vector3.Zero)
            throw new InvalidDataException($"Asset '{asset}': child scenes/scene offset are not supported; use translated entity parents.");
        var result=new List<Entity>();var ids=new HashSet<Guid>();var queue=new Queue<Entity>(scene.Entities);
        while(queue.TryDequeue(out var entity))
        {
            if(entity.Id==Guid.Empty||!ids.Add(entity.Id))throw Error(asset,entity,"Id","empty/duplicate native identity or hierarchy cycle");
            result.Add(entity);
            foreach(var child in entity.Transform.Children)queue.Enqueue(child.Entity);
        }
        return result.ToArray();
    }
    public static Vector3 Position(Entity entity,string asset)
    {
        var p=PrecisePosition(entity,asset);
        return new((float)p.X,(float)p.Y,(float)p.Z);
    }
    private static (double X,double Y,double Z) PrecisePosition(Entity entity,string asset)
    {
        (double X,double Y,double Z) sum=(0,0,0);var visited=new HashSet<Entity>();
        for(var transform=entity.Transform;transform is not null;transform=transform.Parent)
        {
            if(!visited.Add(transform.Entity))throw Error(asset,entity,"Parent","cyclic hierarchy");
            if(!transform.UseTRS||transform.Rotation!=Quaternion.Identity||transform.Scale!=Vector3.One)
                throw Error(asset,transform.Entity,"Transform","requires TRS, identity rotation and unit scale for gameplay geometry and points");
            sum=(sum.X+transform.Position.X,sum.Y+transform.Position.Y,sum.Z+transform.Position.Z);
            if(!Finite(transform.Position)||Math.Abs(sum.X)>float.MaxValue||Math.Abs(sum.Y)>float.MaxValue||Math.Abs(sum.Z)>float.MaxValue)
                throw Error(asset,transform.Entity,"Position","must remain finite including parents");
        }
        return sum;
    }
    public static bool Finite(Vector3 value)=>float.IsFinite(value.X)&&float.IsFinite(value.Y)&&float.IsFinite(value.Z);
    public static void PositiveSize(Entity entity,Vector3 size,string asset)
    {
        if(!Finite(size)||size.X<=0||size.Y<=0||size.Z<=0)throw Error(asset,entity,"Size","requires finite positive X/Y/Z");
    }
    public static (WorldBox? Box,RampDefinition? Ramp) Geometry(GeometryComponent component,string asset)
    {
        // Round only the final bounds. Rounding translated centres to float first
        // breaks shared floor/wall contact (e.g. common parent Y=.1), even when
        // their authored faces coincide exactly. Core's strict contact stays intact.
        var entity=component.Entity;var p=PrecisePosition(entity,asset);var size=component.Size;PositiveSize(entity,size,asset);
        if(!Enum.IsDefined(component.Role)||!Enum.IsDefined(component.Axis))throw Error(asset,entity,"Role/Axis","unknown value");
        string id=entity.Id.ToString();
        if(component.Role==GeometryRole.Ramp)
        {
            var ramp=new RampDefinition(id,new((float)p.X,(float)p.Z),new((float)(p.X+size.X),(float)(p.Z+size.Z)),component.Axis,(float)p.Y,(float)(p.Y+component.Rise),size.Y);
            try {ramp.ToWorld();}catch(Exception error){throw Error(asset,entity,"Ramp",error.Message);}
            return(null,ramp);
        }
        var min=new Vector3((float)(p.X-size.X/2d),(float)(p.Y-size.Y/2d),(float)(p.Z-size.Z/2d));
        var max=new Vector3((float)(p.X+size.X/2d),(float)(p.Y+size.Y/2d),(float)(p.Z+size.Z/2d));
        if(!Finite(min)||!Finite(max)||min.X>=max.X||min.Y>=max.Y||min.Z>=max.Z)
            throw Error(asset,entity,"Size/Position","bounds overflow or collapse at this magnitude");
        return(new(id,Point(min),Point(max)),null);
    }
    public static Point3 Point(Vector3 p)=>new(p.X,p.Y,p.Z);

    public static SceneDefinition Convert(Scene scene,string asset,IReadOnlyDictionary<string,Scene> scenes)
    {
        var entities=Entities(scene,asset);var owned=entities.ToHashSet();
        var roots=entities.Where(e=>e.Get<TraversalSceneComponent>() is not null).ToArray();
        if(roots.Length!=1)throw new InvalidDataException($"Asset '{asset}': requires exactly one Expedition scene component.");
        var root=roots[0];var settings=root.Get<TraversalSceneComponent>();
        Entity Reference(Entity? target,Entity owner,string field)
        {
            if(target is null||!owned.Contains(target))throw Error(asset,owner,field,"missing reference or entity belongs to another scene");
            return target;
        }
        Point3 At(Entity? target,Entity owner,string field)=>Point(Position(Reference(target,owner,field),asset));
        var defaultSpawn=Reference(settings.DefaultSpawn,root,"DefaultSpawn");
        if(defaultSpawn.Get<SpawnComponent>() is null)throw Error(asset,root,"DefaultSpawn","target requires Expedition spawn component");
        var geometry=entities.Where(e=>e.Get<GeometryComponent>() is not null).Select(e=>(Component:e.Get<GeometryComponent>(),Shape:Geometry(e.Get<GeometryComponent>(),asset))).ToArray();
        WorldBox[] Boxes(GeometryRole role)=>geometry.Where(g=>g.Component.Role==role).Select(g=>g.Shape.Box!).ToArray();
        var floors=Boxes(GeometryRole.Floor);
        if(floors.Length!=1)throw Error(asset,root,"Floor","requires exactly one geometry with Floor role");
        foreach(var wall in Boxes(GeometryRole.Wall))
            if(wall.Min.Y!=floors[0].Max.Y)throw Error(asset,entities.Single(e=>e.Id.ToString()==wall.Id),"Position/Size.Y","wall bottom must meet floor top; use Structure role for a raised box");
        if(!float.IsFinite(settings.RecoveryThreshold)||settings.RecoveryThreshold>=floors[0].Min.Y)
            throw Error(asset,root,"RecoveryThreshold","must be finite and below floor bottom");
        var ladders=entities.Where(e=>e.Get<LadderComponent>() is not null).Select(e=>{
            var c=e.Get<LadderComponent>();return new LadderDefinition(e.Id.ToString(),At(c.Bottom,e,"Bottom"),At(c.Top,e,"Top"),At(c.BottomEntry,e,"BottomEntry"),At(c.TopEntry,e,"TopEntry"),At(c.BottomExit,e,"BottomExit"),At(c.TopExit,e,"TopExit"));
        }).ToArray();
        var portals=entities.Where(e=>e.Get<PortalComponent>() is not null).Select(e=>{
            var c=e.Get<PortalComponent>();PositiveSize(e,c.Size,asset);var p=Position(e,asset);
            if(c.TargetScene is null||!scenes.TryGetValue(c.TargetScene.Url,out var target))throw Error(asset,e,"TargetScene","missing native scene reference in project");
            var targetEntities=Entities(target,c.TargetScene.Url);
            var targetRoots=targetEntities.Where(t=>t.Get<TraversalSceneComponent>() is not null).ToArray();
            if(targetRoots.Length!=1)throw Error(asset,e,"TargetScene","target requires exactly one scene component");
            var spawn=c.TargetSpawnId==Guid.Empty?targetRoots[0].Get<TraversalSceneComponent>().DefaultSpawn:
                targetEntities.SingleOrDefault(t=>t.Id==c.TargetSpawnId);
            if(spawn is null||!targetEntities.Contains(spawn)||spawn.Get<SpawnComponent>() is null)throw Error(asset,e,"TargetSpawnId","missing target spawn entity");
            return new PortalDefinition(e.Id.ToString(),Point(p-c.Size/2),Point(p+c.Size/2),Point(p),targetRoots[0].Id.ToString(),spawn.Id.ToString());
        }).ToArray();
        var groups=entities.Where(e=>e.Get<OcclusionComponent>() is not null).Select(e=>{
            var c=e.Get<OcclusionComponent>();
            if(c.Members is null)throw Error(asset,e,"Members","missing collection");
            var members=c.Members.Select(m=>Reference(m,e,"Members").Id.ToString()).ToArray();
            Entity? parent=c.HideWith is null?null:Reference(c.HideWith,e,"HideWith");
            if(parent is not null&&parent.Get<OcclusionComponent>() is null)throw Error(asset,e,"HideWith","target requires occlusion component");
            return new OccluderGroup(e.Id.ToString(),members,parent?.Id.ToString());
        }).ToArray();
        var definition=new SceneDefinition(3,root.Id.ToString(),floors[0],
            entities.Where(e=>e.Get<SpawnComponent>() is not null).Select(e=>new NamedSpawn(e.Id.ToString(),Point(Position(e,asset)))).ToArray(),
            Boxes(GeometryRole.Wall),Boxes(GeometryRole.Structure),ladders,geometry.Where(g=>g.Shape.Ramp is not null).Select(g=>g.Shape.Ramp!).ToArray(),
            portals,settings.RecoveryThreshold,groups,Boxes(GeometryRole.Decoration))
        {DefaultSpawnId=defaultSpawn.Id.ToString(),DisplayNames=entities.ToDictionary(e=>e.Id.ToString(),e=>e.Name)};
        try {definition.Validate();}
        catch(Exception error){
            string message=error.Message;
            foreach(var entity in entities.Where(e=>message.Contains(e.Id.ToString(),StringComparison.Ordinal)))
                message=message.Replace(entity.Id.ToString(),$"{entity.Name} ({entity.Id})",StringComparison.Ordinal);
            throw new InvalidDataException($"Asset '{asset}', gameplay properties/support/corridor validation: {message}",error);
        }
        return definition;
    }
}
